name: Generar módulo ESP-IDF
description: Crea un módulo .h/.c FreeRTOS (tarea sensor + cola + UART) y explica decisiones
invokable: true
---
Genera un módulo ESP-IDF completo y modular para ESP32-S3:

**Requisitos**
- Tarea `sensor_reader_task`: lee NTC (ADC) y potenciómetro; envía `{float T_C; int pot_mV;}` por cola cada 200 ms.
- Tarea `uart_monitor_task`: recibe de la cola y reporta por UART con formato claro.
- Manejo de errores ADC; conversión NTC (β simplificada o tabla).
- Estructura por archivos: 
  - `include/sensor_module.h`
  - `src/sensor_module.c`
  - `src/uart_monitor.c`
  - `CMakeLists.txt` mínimo del componente
- Comentarios y TODO para pines, canal ADC y coeficientes NTC.
- Sugerir prioridades y tamaño de stack con justificación.
- Explicar brevemente por qué esas prioridades/delays/stack evitan watchdog y latencias.

Recuerda: bloques de código indicando lenguaje y ruta, p. ej. ```c src/sensor_module.c
