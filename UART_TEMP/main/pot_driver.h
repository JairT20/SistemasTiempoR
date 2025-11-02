#ifndef CONTROLADOR_POTENCIOMETRO_H
#define CONTROLADOR_POTENCIOMETRO_H

#include "esp_adc/adc_oneshot.h" // Librería para usar el conversor Analógico-Digital (ADC).


void inicializar_potenciometro(adc_oneshot_unit_handle_t manejador_adc);


int leer_potenciometro_mv(adc_oneshot_unit_handle_t manejador_adc);

#endif // CONTROLADOR_POTENCIOMETRO_H