#ifndef CONTROLADOR_LED_RGB_H
#define CONTROLADOR_LED_RGB_H

#include "driver/ledc.h" // Librería para control de LED con PWM (LEDC).


void inicializar_led_rgb(void);

void establecer_color_rgb(uint32_t ciclo_rojo, uint32_t ciclo_verde, uint32_t ciclo_azul);

#endif // CONTROLADOR_LED_RGB_H