#ifndef CONTROLADOR_BOTON_H
#define CONTROLADOR_BOTON_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Constante para el número del pin GPIO al que está conectado el botón.
#define GPIO_BOTON 42

// Inicializa el GPIO y la interrupción del botón.
// Envía un mensaje a la cola 'button_queue' cuando se presiona.
void inicializar_boton(QueueHandle_t cola_eventos_boton);

#endif // CONTROLADOR_BOTON_H


