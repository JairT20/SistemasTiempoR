#include "http_server.h"
#include "time_service.h"

#include "esp_http_server.h"
#include "esp_log.h"

#include <string.h>
#include <time.h>

static const char *TAG = "http_server";
static httpd_handle_t s_server = NULL;

/* ========= Archivos embebidos (index.html, app.js, app.css) ========== */

extern const uint8_t index_html_start[] asm("_binary_webpage_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_webpage_index_html_end");

extern const uint8_t app_js_start[]     asm("_binary_webpage_app_js_start");
extern const uint8_t app_js_end[]       asm("_binary_webpage_app_js_end");

extern const uint8_t app_css_start[]    asm("_binary_webpage_app_css_start");
extern const uint8_t app_css_end[]      asm("_binary_webpage_app_css_end");

/* ========================= Handlers estáticos ========================= */

static esp_err_t root_get_handler(httpd_req_t *req)
{
    const size_t html_size = index_html_end - index_html_start;

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, html_size);

    return ESP_OK;
}

static esp_err_t app_js_get_handler(httpd_req_t *req)
{
    const size_t js_size = app_js_end - app_js_start;

    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)app_js_start, js_size);

    return ESP_OK;
}

static esp_err_t app_css_get_handler(httpd_req_t *req)
{
    const size_t css_size = app_css_end - app_css_start;

    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)app_css_start, css_size);

    return ESP_OK;
}

/* ========================= /time.json ========================= */

static esp_err_t http_get_time_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Petición /time.json");

    char buffer[64];
    struct tm timeinfo;

    // NO BLOQUEAR AQUÍ
    time_service_get_localtime(&timeinfo);

    snprintf(buffer, sizeof(buffer),
             "{\"hour\": %02d, \"min\": %02d, \"sec\": %02d}",
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buffer, strlen(buffer));

    return ESP_OK;
}

/* ==================== /toogle_led.json (POST) ==================== */

static esp_err_t http_post_toggle_led_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Petición /toogle_led.json recibida");
    const char *resp = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

/* ==================== /uart_off.json (POST) ==================== */

static esp_err_t http_post_uart_off_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Petición /uart_off.json recibida");
    const char *resp = "{\"status\":\"uart_off\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

/* ========================= Arranque servidor ========================= */

void http_server_start(void)
{
    if (s_server != NULL) {
        ESP_LOGW(TAG, "HTTP server ya estaba iniciado");
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Iniciando HTTP server en puerto %d...", config.server_port);

    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando HTTP server: %s", esp_err_to_name(ret));
        s_server = NULL;
        return;
    }

    httpd_uri_t root_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = root_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &root_uri);

    httpd_uri_t app_js_uri = {
        .uri      = "/app.js",
        .method   = HTTP_GET,
        .handler  = app_js_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &app_js_uri);

    httpd_uri_t app_css_uri = {
        .uri      = "/app.css",
        .method   = HTTP_GET,
        .handler  = app_css_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &app_css_uri);

    httpd_uri_t time_uri = {
        .uri      = "/time.json",
        .method   = HTTP_GET,
        .handler  = http_get_time_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &time_uri);

    httpd_uri_t toggle_led_uri = {
        .uri      = "/toogle_led.json",
        .method   = HTTP_POST,
        .handler  = http_post_toggle_led_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &toggle_led_uri);

    httpd_uri_t uart_off_uri = {
        .uri      = "/uart_off.json",
        .method   = HTTP_POST,
        .handler  = http_post_uart_off_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uart_off_uri);

    ESP_LOGI(TAG, "HTTP server iniciado y handlers registrados");
}
