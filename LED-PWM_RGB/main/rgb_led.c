// rgb_led.c

#include "rgb_led.h"

// --- Configuración del temporizador PWM (compartido para todos los LEDs) ---
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES       LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY      (4000)

// Variable para asegurar que el timer se inicialice solo una vez
static bool timer_is_initialized = false;

void rgb_led_timer_init(void)
{
    if (timer_is_initialized) {
        return; // No hacer nada si ya está inicializado
    }

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    timer_is_initialized = true;
}

static esp_err_t configure_single_channel(int gpio_num, ledc_channel_t channel)
{
    ledc_channel_config_t ledc_channel_conf = {
        .speed_mode     = LEDC_MODE,
        .channel        = channel,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = gpio_num,
        .duty           = 0,
        .hpoint         = 0
    };
    return ledc_channel_config(&ledc_channel_conf);
}

esp_err_t rgb_led_config(const rgb_led_t *led)
{
    ESP_ERROR_CHECK(configure_single_channel(led->gpio_r, led->channel_r));
    ESP_ERROR_CHECK(configure_single_channel(led->gpio_g, led->channel_g));
    ESP_ERROR_CHECK(configure_single_channel(led->gpio_b, led->channel_b));
    return ESP_OK;
}

esp_err_t set_rgb_color(const rgb_led_t *led, uint32_t r, uint32_t g, uint32_t b)
{
    ledc_set_duty(LEDC_MODE, led->channel_r, r);
    ledc_update_duty(LEDC_MODE, led->channel_r);

    ledc_set_duty(LEDC_MODE, led->channel_g, g);
    ledc_update_duty(LEDC_MODE, led->channel_g);

    ledc_set_duty(LEDC_MODE, led->channel_b, b);
    ledc_update_duty(LEDC_MODE, led->channel_b);

    return ESP_OK;
}

uint32_t map_rgb_to_duty(uint8_t value_8bit)
{
    return (uint32_t)(value_8bit * 8191) / 255;
}