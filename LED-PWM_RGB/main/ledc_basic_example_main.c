// ledc_basic_example_main.c

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // Necesario para los semáforos
#include "driver/gpio.h"    // Necesario para los botones
#include "rgb_led.h"        // Incluimos nuestra librería

// --- Pines para los botones ---
#define BUTTON_1_GPIO   4
#define BUTTON_2_GPIO   5

// --- Globales para la comunicación entre ISR y Tareas ---
SemaphoreHandle_t button1_sem;
SemaphoreHandle_t button2_sem;

// --- Estructuras de los LEDs (globales para que las tareas las vean) ---
static rgb_led_t led1;
static rgb_led_t led2;

/**
 * @brief Rutina de Servicio de Interrupción (ISR) para los botones.
 * Esta función se ejecuta CADA VEZ que se presiona cualquier botón.
 * ¡DEBE SER MUY RÁPIDA!
 */
static void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    if (gpio_num == BUTTON_1_GPIO) {
        // "Entrega" el semáforo a la tarea del LED 1.
        xSemaphoreGiveFromISR(button1_sem, NULL);
    } else if (gpio_num == BUTTON_2_GPIO) {
        // "Entrega" el semáforo a la tarea del LED 2.
        xSemaphoreGiveFromISR(button2_sem, NULL);
    }
}

/**
 * @brief Tarea que controla el LED 1.
 * Espera una señal (semáforo) del botón 1 para cambiar de color.
 */
void led1_control_task(void *pvParameters) {
    int color_index = 0;
    while(1) {
        // La tarea se duerme aquí hasta que el semáforo es entregado por la ISR.
        if (xSemaphoreTake(button1_sem, portMAX_DELAY) == pdTRUE) {
            printf("Botón 1 presionado! Cambiando color de LED 1.\n");
            
            color_index = (color_index + 1) % 4; // Cicla entre 0, 1, 2, 3

            switch(color_index) {
                case 0: set_rgb_color(&led1, map_rgb_to_duty(255), 0, 0); break;       // Rojo
                case 1: set_rgb_color(&led1, 0, map_rgb_to_duty(255), 0); break;       // Verde
                case 2: set_rgb_color(&led1, 0, 0, map_rgb_to_duty(255)); break;       // Azul
                case 3: set_rgb_color(&led1, 0, 0, 0); break;                           // Apagado
            }
        }
    }
}

/**
 * @brief Tarea que controla el LED 2.
 * Espera una señal (semáforo) del botón 2 para cambiar de color.
 */
void led2_control_task(void *pvParameters) {
    int color_index = 0;
    while(1) {
        // La tarea se duerme aquí hasta que el semáforo es entregado por la ISR.
        if (xSemaphoreTake(button2_sem, portMAX_DELAY) == pdTRUE) {
            printf("Botón 2 presionado! Cambiando color de LED 2.\n");
            
            color_index = (color_index + 1) % 4; // Cicla entre 0, 1, 2, 3

            switch(color_index) {
                case 0: set_rgb_color(&led2, map_rgb_to_duty(255), map_rgb_to_duty(255), 0); break; // Amarillo
                case 1: set_rgb_color(&led2, 0, map_rgb_to_duty(255), map_rgb_to_duty(255)); break; // Cian
                case 2: set_rgb_color(&led2, map_rgb_to_duty(255), 0, map_rgb_to_duty(255)); break; // Magenta
                case 3: set_rgb_color(&led2, 0, 0, 0); break;                                       // Apagado
            }
        }
    }
}


void app_main(void) {
    // --- 1. Configuración de LEDs ---
    rgb_led_timer_init();

    // Rellenamos las estructuras de los LEDs
    led1 = (rgb_led_t){.gpio_r=8, .gpio_g=9, .gpio_b=10, .channel_r=LEDC_CHANNEL_0, .channel_g=LEDC_CHANNEL_1, .channel_b=LEDC_CHANNEL_2};
    led2 = (rgb_led_t){.gpio_r=18, .gpio_g=19, .gpio_b=21, .channel_r=LEDC_CHANNEL_3, .channel_g=LEDC_CHANNEL_4, .channel_b=LEDC_CHANNEL_5};
    
    rgb_led_config(&led1);
    rgb_led_config(&led2);

    // --- 2. Creación de Semáforos ---
    // Creamos semáforos binarios, que actúan como una bandera.
    button1_sem = xSemaphoreCreateBinary();
    button2_sem = xSemaphoreCreateBinary();

    // --- 3. Configuración de Botones e Interrupciones ---
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE; // Interrupción en el flanco de subida (de 0V a 3.3V)
    io_conf.pin_bit_mask = (1ULL << BUTTON_1_GPIO) | (1ULL << BUTTON_2_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0; // Deshabilitamos las resistencias internas porque usas externas
    gpio_config(&io_conf);

    // Instalar el servicio de ISR
    gpio_install_isr_service(0);
    // Añadir un manejador para cada botón
    gpio_isr_handler_add(BUTTON_1_GPIO, button_isr_handler, (void*) BUTTON_1_GPIO);
    gpio_isr_handler_add(BUTTON_2_GPIO, button_isr_handler, (void*) BUTTON_2_GPIO);

    // --- 4. Creación de Tareas ---
    xTaskCreate(led1_control_task, "LED 1 Control Task", 2048, NULL, 10, NULL);
    xTaskCreate(led2_control_task, "LED 2 Control Task", 2048, NULL, 10, NULL);

    printf("Configuración completada. Presiona los botones para cambiar los colores.\n");
}