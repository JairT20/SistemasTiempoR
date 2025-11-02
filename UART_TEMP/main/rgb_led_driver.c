#include "rgb_led_driver.h"
#include "esp_err.h"

// --- CONFIGURACIÓN DE PINES Y PWM ---
#define GPIO_LED_ROJO     10
#define GPIO_LED_VERDE    9
#define GPIO_LED_AZUL     46

#define TEMPORIZADOR_LEDC   LEDC_TIMER_0
#define MODO_LEDC           LEDC_LOW_SPEED_MODE
#define CANAL_LEDC_ROJO     LEDC_CHANNEL_0
#define CANAL_LEDC_VERDE    LEDC_CHANNEL_1
#define CANAL_LEDC_AZUL     LEDC_CHANNEL_2
#define RESOLUCION_LEDC     LEDC_TIMER_10_BIT // 10 bits = valores de 0 a 1023.

void inicializar_led_rgb(void) {
    // 1. Configurar el temporizador que generará la señal PWM.
    ledc_timer_config_t config_temporizador_ledc = {
        .speed_mode       = MODO_LEDC,
        .duty_resolution  = RESOLUCION_LEDC,
        .timer_num        = TEMPORIZADOR_LEDC,
        .freq_hz          = 5000, // Frecuencia de 5 kHz (buena para evitar parpadeos).
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&config_temporizador_ledc);

    // 2. Configurar cada canal de color (Rojo, Verde, Azul).
    ledc_channel_config_t canales_ledc[3] = {
        { .gpio_num = GPIO_LED_ROJO, .speed_mode = MODO_LEDC, .channel = CANAL_LEDC_ROJO, .timer_sel = TEMPORIZADOR_LEDC, .duty = 0, .hpoint = 0 },
        { .gpio_num = GPIO_LED_VERDE, .speed_mode = MODO_LEDC, .channel = CANAL_LEDC_VERDE, .timer_sel = TEMPORIZADOR_LEDC, .duty = 0, .hpoint = 0 },
        { .gpio_num = GPIO_LED_AZUL, .speed_mode = MODO_LEDC, .channel = CANAL_LEDC_AZUL, .timer_sel = TEMPORIZADOR_LEDC, .duty = 0, .hpoint = 0 },
    };

    // Aplica la configuración a los 3 canales.
    for (int i = 0; i < 3; i++) {
        ledc_channel_config(&canales_ledc[i]);
    }
}

void establecer_color_rgb(uint32_t ciclo_rojo, uint32_t ciclo_verde, uint32_t ciclo_azul) {
    // Establece la intensidad (duty cycle) para el canal ROJO.
    ledc_set_duty(MODO_LEDC, CANAL_LEDC_ROJO, ciclo_rojo);
    ledc_update_duty(MODO_LEDC, CANAL_LEDC_ROJO);

    // Establece la intensidad (duty cycle) para el canal VERDE.
    ledc_set_duty(MODO_LEDC, CANAL_LEDC_VERDE, ciclo_verde);
    ledc_update_duty(MODO_LEDC, CANAL_LEDC_VERDE);

    // Establece la intensidad (duty cycle) para el canal AZUL.
    ledc_set_duty(MODO_LEDC, CANAL_LEDC_AZUL, ciclo_azul);
    ledc_update_duty(MODO_LEDC, CANAL_LEDC_AZUL);
}