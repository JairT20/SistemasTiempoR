#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

#include "rgb_led_driver.h"
#include "ntc_driver.h"
#include "pot_driver.h"
#include "button_driver.h"

// Etiqueta para los mensajes de registro
static const char *ETIQUETA_APP = "APP_PRINCIPAL";

// --- Estructuras y Colas ---
typedef struct {
    float temperatura_celsius;
    int voltaje_pot_mv;
} DatosSensor_t;


typedef enum {
    CMD_FIJAR_RANGO,    // Un solo comando para R, G y B
    CMD_ACTIVAR_POT,
    CMD_FIJAR_BRILLO,   // Nuevo comando para fijar brillo estático desde UART
    CMD_LED_APAGADO,    
    CMD_LED_ENCENDIDO
} TipoComando_t;

typedef struct {
    TipoComando_t tipo;
    int indice_color;   // 0=Rojo, 1=Verde, 2=Azul
    float valor1;       // min_temp
    float valor2;       // max_temp

    // creamos esta varible 3
    float valor3;       // Potencia Led
} ComandoApp_t;

// Colas y manejadores
static QueueHandle_t cola_datos_sensor;
static QueueHandle_t cola_comandos;
static QueueHandle_t cola_pulsacion_boton;
static QueueHandle_t cola_reimprimir_menu;
static QueueHandle_t cola_intervalo_tiempo;
static QueueHandle_t cola_solicitud_lectura_pot;



// Prototipos de las Tareas
void tarea_receptora_uart(void *pvParameters);             //  El recepcionista que solo toma pedidos (comandos).
void tarea_lectora_sensor(void *pvParameters);         //El técnico que solo mide cosas (temperatura, potenciómetro).
void tarea_control_led(void *pvParameters);            // El operario principal que mira los pedidos y las mediciones y actúa (cambia el color del LED).
void tarea_impresion_monitor(void *pvParameters);      //El supervisor que muestra el estado en una pantalla.

void app_main(void) {
    
    adc_oneshot_unit_handle_t manejador_adc1;//

    // Creacion de colas
    cola_datos_sensor = xQueueCreate(1, sizeof(DatosSensor_t));
    cola_comandos = xQueueCreate(5, sizeof(ComandoApp_t));
    cola_pulsacion_boton = xQueueCreate(1, sizeof(uint32_t));
    cola_reimprimir_menu = xQueueCreate(1, sizeof(uint32_t));
    cola_intervalo_tiempo = xQueueCreate(1, sizeof(int));
    cola_solicitud_lectura_pot = xQueueCreate(1, sizeof(uint32_t));

    // Inicializacion de perifericos
    inicializar_led_rgb();
    adc_oneshot_unit_init_cfg_t config_inicial_adc1 = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&config_inicial_adc1, &manejador_adc1);
    inicializar_ntc(manejador_adc1);
    inicializar_potenciometro(manejador_adc1);
    inicializar_boton(cola_pulsacion_boton);
    
    //Uart
    uart_config_t config_uart = { .baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, .source_clk = UART_SCLK_DEFAULT };
    uart_driver_install(UART_NUM_0, 256 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &config_uart);
    
    // Creacion de tareas
    ESP_LOGI(ETIQUETA_APP, "Inicialización completa. Creando tareas.");
    xTaskCreate(tarea_receptora_uart, "ReceptorUART", 2048, NULL, 5, NULL);
    // Pasamos el manejador del ADC como parámetro a la tarea lector
    xTaskCreate(tarea_lectora_sensor, "LectorSensor", 2048, (void*)manejador_adc1, 10, NULL);
    xTaskCreate(tarea_control_led, "ControlLED", 2048, NULL, 5, NULL);
    xTaskCreate(tarea_impresion_monitor, "ImpresionMonitor", 2048, NULL, 4, NULL);
}

// TAREA Receptora 
void tarea_receptora_uart(void *pvParameters) {
    static uint8_t datos_recibidos[256];
    while (1) {
        int longitud = uart_read_bytes(UART_NUM_0, datos_recibidos, 255, pdMS_TO_TICKS(20));
        if (longitud > 0) {
            datos_recibidos[longitud] = '\0';
            
            ComandoApp_t comando;
            char caracter_tipo;
            int nuevo_intervalo;

            if (strncmp((char*)datos_recibidos, "help", 4) == 0) {
                uint8_t dummy = 1; xQueueSend(cola_reimprimir_menu, &dummy, 0);
            }
            else if (sscanf((char*)datos_recibidos, "pot %f", &comando.valor3) == 1) {
                if (comando.valor3 >= 0.0f && comando.valor3 <= 100.0f) {
                    comando.tipo = CMD_FIJAR_BRILLO;
                    xQueueSend(cola_comandos, &comando, 0);
                    printf("OK. Brillo fijado al %.1f%%.\n> ", comando.valor3); fflush(stdout);
                } else {
                    printf("Error: El valor para 'pot' debe estar entre 0 y 100.\n> "); fflush(stdout);
                }
            }
            else if (sscanf((char*)datos_recibidos, "%c %f %f", &caracter_tipo, &comando.valor1, &comando.valor2) == 3) {
                comando.tipo = CMD_FIJAR_RANGO;
                bool enviar = true;
                if (caracter_tipo == 'R' || caracter_tipo == 'r') comando.indice_color = 0;
                else if (caracter_tipo == 'G' || caracter_tipo == 'g') comando.indice_color = 1;
                else if (caracter_tipo == 'B' || caracter_tipo == 'b') comando.indice_color = 2;
                else enviar = false;
                
                if (enviar) {
                    xQueueSend(cola_comandos, &comando, 0);
                    printf("OK. Rango %c fijado. Control por temperatura activado.\n> ", caracter_tipo); fflush(stdout);
                }
            }
            else if (sscanf((char*)datos_recibidos, "tiempo %d", &nuevo_intervalo) == 1) {
                if (nuevo_intervalo > 0) {
                    xQueueSend(cola_intervalo_tiempo, &nuevo_intervalo, 0);
                    printf("OK. Intervalo cambiado a %d ciclos.\n> ", nuevo_intervalo); fflush(stdout);
                }
            }
            else if (strncmp((char*)datos_recibidos, "volt", 4) == 0) {
                uint8_t dummy = 1; xQueueSend(cola_solicitud_lectura_pot, &dummy, 0);
            }
        }
    }
}

// La tarea lectora de sensor 
void tarea_lectora_sensor(void *pvParameters) {
    // Recibimos el manejador del ADC desde el parámetro de la tarea 
    adc_oneshot_unit_handle_t manejador_adc = (adc_oneshot_unit_handle_t)pvParameters;

    #define MUESTRAS_PROMEDIO 10
    float historial_temp[MUESTRAS_PROMEDIO] = {0};
    int indice_historial = 0;
    while (1) {
        
        historial_temp[indice_historial] = leer_temperatura_celsius(manejador_adc);
        indice_historial = (indice_historial + 1) % MUESTRAS_PROMEDIO;
        
        float suma_temp = 0;
        for (int i = 0; i < MUESTRAS_PROMEDIO; i++) suma_temp += historial_temp[i];
        
        DatosSensor_t datos_actuales;
        datos_actuales.temperatura_celsius = suma_temp / MUESTRAS_PROMEDIO;
        
        datos_actuales.voltaje_pot_mv = leer_potenciometro_mv(manejador_adc);
        
        xQueueOverwrite(cola_datos_sensor, &datos_actuales);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

typedef struct {
    float min;
    float max;
} RangoTemperatura_t;

const uint32_t CICLO_MAXIMO = (1 << 10) - 1;
uint32_t calcular_ciclo_temperatura(float temp_actual, RangoTemperatura_t rango) {
    if (temp_actual >= rango.min && temp_actual <= rango.max) {
        return (uint32_t)(CICLO_MAXIMO * (temp_actual - rango.min) / (rango.max - rango.min));
    }
    return 0;
}

// Tarea de Control 
void tarea_control_led(void *pvParameters) {
    //Rangos de inicio prestablecidos
    RangoTemperatura_t rangos_rgb[3] = {
        { .min = 0, .max = 15 },    
        { .min = 10, .max = 30 },   
        { .min = 25, .max = 40 }    
    };
    bool control_por_potenciometro = false;
    bool led_esta_pausado = false;
    DatosSensor_t sensores_actuales;
    float brillo_fijo_porcentaje = -1.0; 
    ComandoApp_t comando_recibido;

    while (1) {
        if (xQueueReceive(cola_comandos, &comando_recibido, 0) == pdPASS) {
            switch(comando_recibido.tipo) {
                case CMD_FIJAR_RANGO:
                    control_por_potenciometro = false;
                    brillo_fijo_porcentaje = -1.0; 
                    int idx = comando_recibido.indice_color;
                    rangos_rgb[idx].min = comando_recibido.valor1;
                    rangos_rgb[idx].max = comando_recibido.valor2;
                    if (rangos_rgb[idx].min >= rangos_rgb[idx].max) {
                        rangos_rgb[idx].max = rangos_rgb[idx].min + 0.1f;
                    }
                    break;
                case CMD_ACTIVAR_POT:
                    control_por_potenciometro = true;
                    brillo_fijo_porcentaje = -1.0; 
                    break;
                case CMD_FIJAR_BRILLO:
                    brillo_fijo_porcentaje = comando_recibido.valor3;
                    control_por_potenciometro = false; 
                    break;
                case CMD_LED_APAGADO: led_esta_pausado = true; establecer_color_rgb(0, 0, 0); break;
                case CMD_LED_ENCENDIDO: led_esta_pausado = false; break;
            }
        }

        if (led_esta_pausado) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (xQueuePeek(cola_datos_sensor, &sensores_actuales, 0) == pdPASS) {
            uint32_t ciclos_rgb[3];

            if (brillo_fijo_porcentaje >= 0.0) {
                uint32_t ciclo_fijo = (uint32_t)(CICLO_MAXIMO * brillo_fijo_porcentaje / 100.0f);
                ciclos_rgb[0] = ciclo_fijo;
                ciclos_rgb[1] = ciclo_fijo;
                ciclos_rgb[2] = ciclo_fijo;
            } else if (control_por_potenciometro) {
                uint32_t ciclo_pot = (uint32_t)(sensores_actuales.voltaje_pot_mv * CICLO_MAXIMO) / 3300;
                ciclos_rgb[0] = ciclo_pot;
                ciclos_rgb[1] = ciclo_pot;
                ciclos_rgb[2] = ciclo_pot;
            } else {
                for (int i = 0; i < 3; i++) {
                    ciclos_rgb[i] = calcular_ciclo_temperatura(sensores_actuales.temperatura_celsius, rangos_rgb[i]);
                }
            }
            establecer_color_rgb(ciclos_rgb[0], ciclos_rgb[1], ciclos_rgb[2]);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// Tarea de Impresión y Menú 
static void imprimir_menu(void) {
    printf("\n\n=== MODO CONFIGURACIÓN ===\n");
    printf("El monitor de sensores está en pausa. Puedes enviar comandos:\n");
    printf("----------------------------------------------------------\n");
    printf("  R <min> <max>   -> Define el rango de Temp. para el ROJO\n");
    printf("  G <min> <max>   -> Define el rango de Temp. para el VERDE\n");
    printf("  B <min> <max>   -> Define el rango de Temp. para el AZUL\n");
    printf("  pot <0-100>     -> Fija un brillo estático (ej: pot 75.5)\n");
    printf("  pot             -> (No implementado, usar 'pot <valor>')\n");
    printf("  (Fijar un rango R,G,B devuelve el control a la temperatura)\n");
    printf("  tiempo <ciclos> -> Modifica el tiempo de impresion (ej: tiempo 10)\n");
    printf("  volt            -> Solicita una lectura del potenciómetro\n");
    printf("  help            -> Vuelve a mostrar este menú de ayuda\n");
    printf("----------------------------------------------------------\n");
    printf(">>> Presiona el BOTÓN FÍSICO para volver al modo monitor <<<\n\n> ");
    fflush(stdout);
}

void tarea_impresion_monitor(void *pvParameters) {
    bool esta_monitoreando = true;
    uint32_t gpio_boton;
    uint8_t notificacion_ficticia;
    int cuenta_regresiva_impresion = 0;
    int intervalo_impresion_ciclos = 20;

    printf("\nSistema inicializado. Modo Monitor: ACTIVADO\n");

    while (1) {
        int nuevo_intervalo_recibido;
        if (xQueueReceive(cola_intervalo_tiempo, &nuevo_intervalo_recibido, 0) == pdPASS) {
            intervalo_impresion_ciclos = nuevo_intervalo_recibido;
        }

        if (xQueueReceive(cola_pulsacion_boton, &gpio_boton, 0) == pdPASS) {
            esta_monitoreando = !esta_monitoreando;
            ComandoApp_t comando;
            
            if (esta_monitoreando) {
                printf("\n\n--- Modo Monitor: ACTIVADO ---\n");
                comando.tipo = CMD_LED_ENCENDIDO;
                xQueueSend(cola_comandos, &comando, 0);
                cuenta_regresiva_impresion = 0;
            } else {
                imprimir_menu();
                comando.tipo = CMD_LED_APAGADO;
                xQueueSend(cola_comandos, &comando, 0);
            }
        }

        if (!esta_monitoreando && xQueueReceive(cola_reimprimir_menu, &notificacion_ficticia, 0) == pdPASS) {
             imprimir_menu();
        }

        if (xQueueReceive(cola_solicitud_lectura_pot, &notificacion_ficticia, 0) == pdPASS) {
            DatosSensor_t datos_actuales;
            if (xQueuePeek(cola_datos_sensor, &datos_actuales, 0) == pdPASS) {
                printf("\n> Lectura Potenciómetro: %d mV\n> ", datos_actuales.voltaje_pot_mv);
            } else {
                printf("\n> [AVISO] Datos del sensor aún no disponibles.\n> ");
            }
            fflush(stdout);
        }

        if (esta_monitoreando) {
            cuenta_regresiva_impresion--;
            if (cuenta_regresiva_impresion <= 0) {
                DatosSensor_t datos;
                if (xQueuePeek(cola_datos_sensor, &datos, 0) == pdPASS) {
                    printf("Temp: %.2f C | Pot: %d mV\n", datos.temperatura_celsius, datos.voltaje_pot_mv);
                } else {
                    printf("[AVISO] Esperando datos del sensor...\n");
                }
                cuenta_regresiva_impresion = intervalo_impresion_ciclos;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}