#include "button_driver.h"
#include "driver/gpio.h"

// Variable estática para almacenar el manejador de la cola que viene desde la aplicación principal.
static QueueHandle_t cola_gpio_eventos = NULL;

static void IRAM_ATTR manejador_isr_boton(void* arg) {
    uint32_t numero_gpio = (uint32_t) arg;
    // Envía el número del pin a la cola desde la ISR. Es una función segura para interrupciones.
    xQueueSendFromISR(cola_gpio_eventos, &numero_gpio, NULL);
}

void inicializar_boton(QueueHandle_t cola_eventos_boton) {
    // Guarda el manejador de la cola en nuestra variable estática para que la ISR pueda usarla.
    cola_gpio_eventos = cola_eventos_boton;

    // 1. Configurar el pin GPIO.
    gpio_config_t configuracion_io = {
        .intr_type = GPIO_INTR_NEGEDGE,             // Tipo de interrupción: flanco de bajada (al presionar).
        .pin_bit_mask = (1ULL << GPIO_BOTON),       // Máscara de bits para seleccionar el pin (GPIO 42).
        .mode = GPIO_MODE_INPUT,                    // Modo del pin: Entrada.
        .pull_up_en = 1,                            // Habilitar la resistencia de pull-up interna.
        .pull_down_en = 0,                          // Deshabilitar la resistencia de pull-down.
    };
    gpio_config(&configuracion_io); // Aplica la configuración.

    // 2. Instalar el servicio global de interrupciones para los GPIO.
    gpio_install_isr_service(0);

    // 3. Añadir un manejador de interrupción específico para nuestro pin.
    gpio_isr_handler_add(GPIO_BOTON, manejador_isr_boton, (void*) GPIO_BOTON);
}