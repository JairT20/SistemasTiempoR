name: Inicio FreeRTOS
description: Analiza tareas, prioridades, colas y riesgos (watchdog, latencia, stack)
invokable: true
---
Analiza el proyecto actual (archivos abiertos y del workspace) y entrega un diagnóstico técnico de FreeRTOS:

1) **Mapa de tareas**: nombre/función, prioridad sugerida, periodo (delay), tamaño de stack recomendado, recursos que usa (colas, semáforos, mutex, eventos), ISR asociadas.
2) **Problemas potenciales**: bloqueos, starvation, prioridades mal ordenadas, ausencia de vTaskDelay, espera activa, uso excesivo de CPU, colas saturadas.
3) **Riesgos**: "Task watchdog triggered", "Guru Meditation", desbordes de stack, brownout por consumo, ISR demasiado larga.
4) **Optimización propuesta**: cambios en prioridades, periodos, tamaños de stack, uso de eventos/colas, separación de responsabilidades, sugerencias de medición (tiempos, logs UART).
5) **Plan de acción**: 
   - Críticas (rompen el sistema)
   - Importantes (mejoran rendimiento/consumo)
   - Opcionales (limpieza/claridad)

Si faltan datos, declara hipótesis razonables. Responde en español, directo y con pasos.
