#include "time_service.h"

#include "esp_sntp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "time_service";

static volatile bool s_time_synced = false;

static void time_sync_notification_cb(struct timeval *tv)
{
    s_time_synced = true;
    ESP_LOGI(TAG, "Hora sincronizada desde servidor NTP");
}

void time_service_init(const char *ntp_server)
{
    if (esp_sntp_enabled()) {
        ESP_LOGW(TAG, "SNTP ya estaba inicializado");
        return;
    }

    // (Opcional) zona horaria por defecto
    // Colombia: UTC-5 (sin horario de verano)
    // Puedes cambiarla luego con time_service_set_timezone()
    setenv("TZ", "COT-5", 1);
    tzset();

    // Configurar SNTP en modo POLL (no bloqueante)
    esp_sntp_config_t config = ESP_SNTP_DEFAULT_CONFIG(ntp_server && ntp_server[0] != '\0'
                                                       ? ntp_server
                                                       : "pool.ntp.org");

    config.sync_cb = time_sync_notification_cb;
    config.smooth_sync = false; // ajuste inmediato

    esp_sntp_init(&config);

    ESP_LOGI(TAG, "SNTP inicializado con servidor: %s",
             ntp_server && ntp_server[0] != '\0' ? ntp_server : "pool.ntp.org");
}

bool time_service_wait_for_sync(uint32_t timeout_ms)
{
    const uint32_t delay_step_ms = 200;
    uint32_t waited_ms = 0;

    while (!s_time_synced && waited_ms < timeout_ms) {
        vTaskDelay(pdMS_TO_TICKS(delay_step_ms));
        waited_ms += delay_step_ms;
    }

    if (s_time_synced) {
        ESP_LOGI(TAG, "Tiempo sincronizado en ~%u ms", waited_ms);
        return true;
    } else {
        ESP_LOGW(TAG, "No se logró sincronizar la hora en %u ms", timeout_ms);
        return false;
    }
}

time_t time_service_get_epoch(void)
{
    time_t now;
    time(&now);          // NO BLOQUEA. Si no hay SNTP, cuenta desde 1-1-1970.
    return now;
}

void time_service_get_localtime(struct tm *info)
{
    if (!info) return;

    time_t now = time_service_get_epoch();

    // Siempre llenamos la struct con algo (aunque la fecha sea 1970)
    localtime_r(&now, info);

    // Puedes imprimir en debug si quieres ver que se está llamando:
    // ESP_LOGD(TAG, "localtime: %02d:%02d:%02d",
    //          info->tm_hour, info->tm_min, info->tm_sec);
}

void time_service_set_timezone(const char *tz_string)
{
    if (!tz_string) return;

    setenv("TZ", tz_string, 1);
    tzset();
    ESP_LOGI(TAG, "Zona horaria establecida a: %s", tz_string);
}

void time_service_print_current_time(void)
{
    struct tm info;
    time_service_get_localtime(&info);

    int year = info.tm_year + 1900;

    if (year < 2020) {
        ESP_LOGW(TAG,
                 "La hora parece no estar bien sincronizada (año=%d). "
                 "¿Tiene acceso a NTP?",
                 year);
    }

    ESP_LOGI(TAG, "Hora actual: %04d-%02d-%02d %02d:%02d:%02d",
             year,
             info.tm_mon + 1,
             info.tm_mday,
             info.tm_hour,
             info.tm_min,
             info.tm_sec);
}
