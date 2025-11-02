#ifndef LED_H
#define LED_H

#include "driver/ledc.h"

typedef struct LED
{
    int gpio_num;
    ledc_channel_t channel;
    uint32_t duty;

} LED_T;

typedef struct LED_RGB {
    LED_T red;
    LED_T green;
    LED_T blue;
    ledc_timer_t timer;
    ledc_mode_t speed_mode; // Modo de velocidad (alta o baja)

} LED_RGB_T;

// Prototipos de funciones 

// Configura un LED RGB completo
LED_RGB_T configure_LED_RGB(int gpio_num_red, int gpio_num_green, int gpio_num_blue,
                            ledc_channel_t channel_red, ledc_channel_t channel_green, ledc_channel_t channel_blue,
                            ledc_timer_t timer, ledc_timer_bit_t duty_resolution, int frequency);

// Establece el color del LED RGB
void set_color(LED_RGB_T* led_rgb, int red_duty, int green_duty, int blue_duty);

// Funciones para establecer el color de cada canal individualmente
void set_red(LED_RGB_T* led_rgb, int red_duty);
void set_green(LED_RGB_T* led_rgb, int green_duty);
void set_blue(LED_RGB_T* led_rgb, int blue_duty);

#endif 