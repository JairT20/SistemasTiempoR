#include "pot_driver.h"

#define CANAL_ADC_POT         ADC_CHANNEL_5 // Canal del ADC donde está conectado el potenciómetro.
#define VOLTAJE_REFERENCIA_MV 3300          // Voltaje de operación del microcontrolador (3.3V = 3300mV).

void inicializar_potenciometro(adc_oneshot_unit_handle_t manejador_adc) {
    // Configura el canal del ADC específico para el potenciómetro.
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,           // Atenuación para medir el rango completo de voltaje.
        .bitwidth = ADC_BITWIDTH_DEFAULT,   // Resolución por defecto (12 bits).
    };
    adc_oneshot_config_channel(manejador_adc, CANAL_ADC_POT, &config);
}

int leer_potenciometro_mv(adc_oneshot_unit_handle_t manejador_adc) {
    int valor_adc_raw;
    // 1. Lee el valor digital del ADC (0-4095).
    adc_oneshot_read(manejador_adc, CANAL_ADC_POT, &valor_adc_raw);
    
    // 2. Convierte el valor digital a milivoltios usando una regla de tres simple.
    return (valor_adc_raw * VOLTAJE_REFERENCIA_MV) / 4095;
}