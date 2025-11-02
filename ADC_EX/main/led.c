#include "led.h"
#include "driver/ledc.h"
#include "esp_err.h" // Necesario para ESP_ERROR_CHECK
#include <stdio.h>

// Función para configurar un LED RGB completo
LED_RGB_T configure_LED_RGB(int gpio_num_red, int gpio_num_green, int gpio_num_blue,
                            ledc_channel_t channel_red, ledc_channel_t channel_green, ledc_channel_t channel_blue,
                            ledc_timer_t timer, ledc_timer_bit_t duty_resolution, int frequency)
{
    LED_RGB_T led_rgb;

    // Asignación de pines, canales y valores iniciales
    led_rgb.red.gpio_num = gpio_num_red;
    led_rgb.red.channel = channel_red;
    led_rgb.red.duty = 0;
    led_rgb.green.gpio_num = gpio_num_green;
    led_rgb.green.channel = channel_green;
    led_rgb.green.duty = 0;
    led_rgb.blue.gpio_num = gpio_num_blue;
    led_rgb.blue.channel = channel_blue;
    led_rgb.blue.duty = 0;
    led_rgb.timer = timer;
    led_rgb.speed_mode = LEDC_LOW_SPEED_MODE; // Asumiendo baja velocidad

    // Configuración del temporizador PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = led_rgb.speed_mode,
        .duty_resolution  = duty_resolution,
        .timer_num        = led_rgb.timer,
        .freq_hz          = frequency,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configuración del canal para el LED ROJO
    ledc_channel_config_t ledc_channel_r = {
        .speed_mode     = led_rgb.speed_mode,
        .channel        = led_rgb.red.channel,
        .timer_sel      = led_rgb.timer,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = led_rgb.red.gpio_num,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_r));

    // Configuración del canal para el LED VERDE
    ledc_channel_config_t ledc_channel_g = {
        .speed_mode     = led_rgb.speed_mode,
        .channel        = led_rgb.green.channel,
        .timer_sel      = led_rgb.timer,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = led_rgb.green.gpio_num,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_g));

    // Configuración del canal para el LED AZUL
    ledc_channel_config_t ledc_channel_b = {
        .speed_mode     = led_rgb.speed_mode,
        .channel        = led_rgb.blue.channel,
        .timer_sel      = led_rgb.timer,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = led_rgb.blue.gpio_num,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_b));

    return led_rgb;
}

// Esta función establece el color del LED RGB
void set_color(LED_RGB_T* led_rgb, int red_duty, int green_duty, int blue_duty)
{
    set_red(led_rgb, red_duty);
    set_green(led_rgb, green_duty);
    set_blue(led_rgb, blue_duty);
}

// Esta función establece el valor de el LED rojo
void set_red(LED_RGB_T* led_rgb, int red_duty)
{
    led_rgb->red.duty = red_duty;
    ESP_ERROR_CHECK(ledc_set_duty(led_rgb->speed_mode, led_rgb->red.channel, led_rgb->red.duty));
    ESP_ERROR_CHECK(ledc_update_duty(led_rgb->speed_mode, led_rgb->red.channel));
}

// Esta función establece el valor de el LED verde
void set_green(LED_RGB_T* led_rgb, int green_duty)
{
    led_rgb->green.duty = green_duty;
    ESP_ERROR_CHECK(ledc_set_duty(led_rgb->speed_mode, led_rgb->green.channel, led_rgb->green.duty));
    ESP_ERROR_CHECK(ledc_update_duty(led_rgb->speed_mode, led_rgb->green.channel));
}

// Esta función establece el valor de el LED azul
void set_blue(LED_RGB_T* led_rgb, int blue_duty)
{
    led_rgb->blue.duty = blue_duty;
    ESP_ERROR_CHECK(ledc_set_duty(led_rgb->speed_mode, led_rgb->blue.channel, led_rgb->blue.duty));
    ESP_ERROR_CHECK(ledc_update_duty(led_rgb->speed_mode, led_rgb->blue.channel));
}