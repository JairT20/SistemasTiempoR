/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" // <--- AÑADIDO: Necesario para usar colas (Queues)
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "led.h"

// --- Definiciones y Constantes (se mantienen igual) ---
const static char *TAG = "NTC_Y_POT_EXAMPLE";
#define NTC_ADC_CHANNEL         ADC_CHANNEL_3
#define POT_ADC_CHANNEL         ADC_CHANNEL_5
#define EXAMPLE_ADC_ATTEN       ADC_ATTEN_DB_12
#define LED_RED_GPIO            10
#define LED_GREEN_GPIO          9
#define LED_BLUE_GPIO           46
#define TEMP_MIN                10.0f
#define TEMP_MAX                30.0f
#define VOLTAGE_DIVIDER_RESISTANCE  10000
#define NTC_NOMINAL_RESISTANCE      10000
#define NTC_NOMINAL_TEMPERATURE     25.0
#define NTC_BETA_COEFFICIENT        3950
#define VOLTAGE_REFERENCE           3300

// --- Estructura de datos para la comunicación entre tareas ---
typedef struct {
    float temperatura_c;
    int pot_voltage_mv;
} SensorData_t;

// --- Variables Globales para los "handles" de ADC y la Cola ---
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t ntc_cali_handle = NULL;
static adc_cali_handle_t pot_cali_handle = NULL;
static bool do_calibration_ntc;
static bool do_calibration_pot;
static QueueHandle_t xSensorDataQueue; // "Buzón" para los datos de los sensores

// --- Prototipos de funciones (se mantienen igual) ---
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);


// =================================================================
// TAREA 1: LEER SENSORES
// =================================================================
void sensor_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea de Sensores iniciada.");
    
    while (1) {
        SensorData_t current_sensor_data = {0}; // Estructura para guardar los datos de esta iteración

        // --- 1. LECTURA DEL NTC Y CÁLCULO DE TEMPERATURA ---
        int ntc_raw_reading;
        int ntc_voltage_mv = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, NTC_ADC_CHANNEL, &ntc_raw_reading));
        if (do_calibration_ntc) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(ntc_cali_handle, ntc_raw_reading, &ntc_voltage_mv));
            if (ntc_voltage_mv > 0 && ntc_voltage_mv < VOLTAGE_REFERENCE) {
                float ntc_resistance = (float)VOLTAGE_DIVIDER_RESISTANCE * ntc_voltage_mv / (VOLTAGE_REFERENCE - ntc_voltage_mv);
                float steinhart = log(ntc_resistance / NTC_NOMINAL_RESISTANCE) / NTC_BETA_COEFFICIENT + 1.0 / (NTC_NOMINAL_TEMPERATURE + 273.15);
                current_sensor_data.temperatura_c = (1.0 / steinhart) - 273.15;
            } else {
                current_sensor_data.temperatura_c = TEMP_MAX; // Valor seguro en caso de error de lectura
            }
        }

        // --- 2. LECTURA DEL POTENCIÓMETRO ---
        int pot_raw_reading;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, POT_ADC_CHANNEL, &pot_raw_reading));
        if (do_calibration_pot) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(pot_cali_handle, pot_raw_reading, &current_sensor_data.pot_voltage_mv));
        }

        // --- 3. ENVIAR LOS DATOS AL "BUZÓN" (Cola) ---
        // xQueueOverwrite asegura que la cola siempre tenga el valor MÁS RECIENTE.
        // Si la tarea del LED es lenta, los datos viejos se descartan, que es lo que queremos.
        xQueueOverwrite(xSensorDataQueue, &current_sensor_data);

        // Esta tarea puede dormir por un tiempo moderado, ya que los sensores no cambian tan rápido.
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}


// =================================================================
// TAREA 2:CONTROLAR EL LED
// =================================================================
void led_control_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea de Control de LED iniciada.");

    // Recibimos el "control remoto" del LED como parámetro
    LED_RGB_T *my_led = (LED_RGB_T *)pvParameters;
    uint32_t max_duty = (1 << LEDC_TIMER_10_BIT) - 1;

    // Inicializamos con valores seguros
    SensorData_t latest_sensor_data = { .temperatura_c = 20.0f, .pot_voltage_mv = 0 };

    while (1) {
        // --- 1. REVISAR SI HAY DATOS NUEVOS EN EL BUZÓN ---
        // El '0' al final significa que no espera. Solo revisa si hay algo y continúa.
        if (xQueueReceive(xSensorDataQueue, &latest_sensor_data, (TickType_t)0) == pdPASS) {
            // ¡Sí, recibimos datos frescos! 'latest_sensor_data' ha sido actualizada.
        }

        // --- 2. LÓGICA DE CONTROL DEL LED (usa los últimos datos recibidos) ---
        uint32_t red_duty = 0;
        if (latest_sensor_data.temperatura_c <= TEMP_MIN) {
            red_duty = 0;
        } else if (latest_sensor_data.temperatura_c >= TEMP_MAX) {
            red_duty = max_duty;
        } else {
            red_duty = (uint32_t)(max_duty * (latest_sensor_data.temperatura_c - TEMP_MIN) / (TEMP_MAX - TEMP_MIN));
        }

        uint32_t green_duty = (uint32_t)(latest_sensor_data.pot_voltage_mv * max_duty) / VOLTAGE_REFERENCE;
        if (green_duty > max_duty) {
            green_duty = max_duty;
        }
        
        uint32_t blue_duty = 0;

        // --- 3. APLICAR LOS VALORES AL LED ---
        set_color(my_led, red_duty, green_duty, blue_duty);

        // --- 4. MOSTRAR INFORMACIÓN EN EL MONITOR SERIE ---
        ESP_LOGI(TAG, "Temp: %.2f C -> Red Duty: %4lu | Pot: %4d mV -> Green Duty: %4lu", 
                 latest_sensor_data.temperatura_c, red_duty, latest_sensor_data.pot_voltage_mv, green_duty);

        // Esta tarea se ejecuta muy rápido para que el control del LED sea fluido.
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}


// =================================================================
// FUNCIÓN PRINCIPAL
// =================================================================
void app_main(void)
{
    // --- 1. CONFIGURACIÓN INICIAL DE HARDWARE (se ejecuta una vez) ---
    adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = { .atten = EXAMPLE_ADC_ATTEN, .bitwidth = ADC_BITWIDTH_DEFAULT };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, NTC_ADC_CHANNEL, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, POT_ADC_CHANNEL, &config));
    
    do_calibration_ntc = example_adc_calibration_init(ADC_UNIT_1, NTC_ADC_CHANNEL, EXAMPLE_ADC_ATTEN, &ntc_cali_handle);
    do_calibration_pot = example_adc_calibration_init(ADC_UNIT_1, POT_ADC_CHANNEL, EXAMPLE_ADC_ATTEN, &pot_cali_handle);
    
    // Configuramos el LED y guardamos su "control remoto" en una variable estática para que no se pierda.
    static LED_RGB_T my_led;
    my_led = configure_LED_RGB(
        LED_RED_GPIO, LED_GREEN_GPIO, LED_BLUE_GPIO,
        LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2,
        LEDC_TIMER_0, LEDC_TIMER_10_BIT, 5000
    );

    // --- 2. CREAR EL MECANISMO DE COMUNICACIÓN (La Cola) ---
    // Creamos una cola que puede contener 1 item del tamaño de nuestra estructura de datos.
    xSensorDataQueue = xQueueCreate(1, sizeof(SensorData_t));
    if(xSensorDataQueue == NULL){
        ESP_LOGE(TAG, "Error al crear la cola de datos de sensores.");
    }

    // --- 3. CREAR Y LANZAR LAS TAREAS ESPECIALISTAS ---
    xTaskCreate(sensor_task,      // Función que implementa la tarea
                "Sensor Task",      // Nombre descriptivo
                2048,               // Tamaño de la pila (stack size) en palabras
                NULL,               // Parámetros para la tarea (no pasamos ninguno)
                5,                  // Prioridad (5 es una prioridad media)
                NULL);              // Handle de la tarea (no lo necesitamos)

    xTaskCreate(led_control_task, // Función de la tarea del LED
                "LED Control Task", // Nombre
                2048,               // Stack size
                &my_led,            // ¡Le pasamos el "control remoto" del LED como parámetro!
                5,                  // Prioridad
                NULL);              // Handle

    ESP_LOGI(TAG, "Inicialización completa. Tareas creadas y en ejecución.");
    // El trabajo de app_main ha terminado. Las tareas ahora corren por su cuenta.
}


/*---------------------------------------------------------------
        ADC Calibration 
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    // ... (código sin cambios)
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
    // ... (código sin cambios)
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}