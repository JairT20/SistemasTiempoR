// rgb_led.h

#ifndef RGB_LED_H
#define RGB_LED_H

#include <stdint.h>
#include "driver/ledc.h" // Necesario para ledc_channel_t
#include "esp_err.h"     // Necesario para esp_err_t

// Estructura que define un LED RGB
typedef struct {
    uint8_t gpio_r;
    uint8_t gpio_g;
    uint8_t gpio_b;
    ledc_channel_t channel_r;
    ledc_channel_t channel_g;
    ledc_channel_t channel_b;
} rgb_led_t;

/**
 * @brief Inicializa el temporizador PWM compartido para todos los LEDs.
 *        Debe llamarse una sola vez al principio.
 */
void rgb_led_timer_init(void);

/**
 * @brief Configura los canales PWM para un LED RGB específico.
 * 
 * @param led Puntero a la estructura del LED que se va a configurar.
 * @return esp_err_t ESP_OK si la configuración fue exitosa.
 */
esp_err_t rgb_led_config(const rgb_led_t *led);

/**
 * @brief Establece el color de un LED RGB específico usando valores de ciclo de trabajo.
 * 
 * @param led Puntero a la estructura del LED cuyo color se va a cambiar.
 * @param r Valor de brillo para el rojo (0-8191).
 * @param g Valor de brillo para el verde (0-8191).
 * @param b Valor de brillo para el azul (0-8191).
 * @return esp_err_t ESP_OK si el color se estableció correctamente.
 */
esp_err_t set_rgb_color(const rgb_led_t *led, uint32_t r, uint32_t g, uint32_t b);

/**
 * @brief Convierte un valor de color de 8 bits (0-255) al rango de ciclo de trabajo (0-8191).
 * 
 * @param value_8bit El valor de color entre 0 y 255.
 * @return uint32_t El valor de ciclo de trabajo correspondiente.
 */
uint32_t map_rgb_to_duty(uint8_t value_8bit);

#endif // RGB_LED_H