#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <stdbool.h>
#include <time.h>
#include <stdint.h>

/**
 * Inicializa el servicio de tiempo:
 *  - Configura el cliente SNTP (no bloqueante).
 *  - Arranca la sincronización con el servidor NTP.
 *
 * Debes llamarlo DESPUÉS de que el ESP32 tenga IP (WiFi conectado)
 * si quieres que realmente llegue al servidor NTP.
 *
 * ntp_server:
 *   - Si es NULL o cadena vacía -> usa "pool.ntp.org"
 *   - Si no, usa el servidor que pases.
 */
void time_service_init(const char *ntp_server);

/**
 * Espera (bloqueante) hasta que la hora esté sincronizada
 * o se agote el timeout.
 *
 * timeout_ms: tiempo máximo de espera en milisegundos.
 * Devuelve:
 *  - true  -> la hora se sincronizó
 *  - false -> no se logró sincronizar en el tiempo dado
 *
 * OJO: en modo AP sin internet, esto casi seguro devolverá false.
 */
bool time_service_wait_for_sync(uint32_t timeout_ms);

/**
 * Devuelve el tiempo actual (epoch, segundos desde 1970).
 * Aunque NO se haya sincronizado con SNTP, el valor se va
 * incrementando (basado en el RTC interno).
 */
time_t time_service_get_epoch(void);

/**
 * Devuelve la hora local en una struct tm.
 * NO bloquea, y siempre rellena timeinfo con algo.
 */
void time_service_get_localtime(struct tm *info);

/**
 * Configura zona horaria (opcional).
 * Ejemplo Colombia (UTC-5 sin DST):
 *   time_service_set_timezone("COT-5");
 */
void time_service_set_timezone(const char *tz_string);

/**
 * Imprime la hora actual por log (para debug).
 */
void time_service_print_current_time(void);

#endif // TIME_SERVICE_H
