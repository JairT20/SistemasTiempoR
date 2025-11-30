#include <string.h>
//#include <arpa/inet.h>
#include "lwip/inet.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "esp_mac.h"

#include "http_server.h"
#include "wifi_app.h"

#include "time_service.h"

// Tag used for ESP serial console messages
static const char TAG[] = "wifi_app";

// Queue handle used to manipulate the main queue of events
static QueueHandle_t wifi_app_queue_handle;

// netif objects for the station and access point
esp_netif_t* esp_netif_sta = NULL;
esp_netif_t* esp_netif_ap  = NULL;

// Optional callback when STA gets IP
static wifi_connected_event_callback_t s_connected_cb = NULL;

// Station configuration (kept so other modules can read/modify)
static wifi_config_t s_sta_config = {
	.sta = {
		.ssid = WIFI_STA_SSID,
		.password = WIFI_STA_PASSWORD,
	},
};

/**
 * WiFi application event handler
 * @param arg data, aside from event data, that is passed to the handler when it is called
 * @param event_base the base id of the event to register the handler for
 * @param event_id the id fo the event to register the handler for
 * @param event_data event data
 */
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "AP: Dispositivo " MACSTR " se ha conectado.", MAC2STR(event->mac));
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "AP: Dispositivo " MACSTR " se ha desconectado.", MAC2STR(event->mac));
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "STA: Modo estación iniciado, conectando a la red...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "STA: Conectado a la red %s", (const char*) s_sta_config.sta.ssid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "STA: Se perdió la conexión. Reintentando...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "STA: IP obtenida: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_app_send_message(WIFI_APP_MSG_STA_CONNECTED_GOT_IP);
    }
}


/**
 * Initializes the WiFi application event handler for WiFi and IP events.
 */
static void wifi_app_event_handler_init(void)
{
	// Event loop for the WiFi driver
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	// Event handler for the connection
	esp_event_handler_instance_t instance_wifi_event;
	esp_event_handler_instance_t instance_ip_event;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_wifi_event));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_ip_event));
}

/**
 * Initializes the TCP stack and default WiFi configuration.
 */
static void wifi_app_default_wifi_init(void)
{
	// Initialize the TCP stack
	ESP_ERROR_CHECK(esp_netif_init());

	// Default WiFi config - operations must be in this order!
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	esp_netif_sta = esp_netif_create_default_wifi_sta();
	esp_netif_ap = esp_netif_create_default_wifi_ap();
}

/**
 * Configures the WiFi access point settings and assigns the static IP to the SoftAP.
 */
static void wifi_app_soft_ap_config(void)
{
	// SoftAP - WiFi access point configuration
	wifi_config_t ap_config =
	{
		.ap = {
				.ssid = WIFI_AP_SSID,
				.ssid_len = strlen(WIFI_AP_SSID),
				.password = WIFI_AP_PASSWORD,
				.channel = WIFI_AP_CHANNEL,
				.ssid_hidden = WIFI_AP_SSID_HIDDEN,
				.authmode = WIFI_AUTH_WPA2_PSK,
				.max_connection = WIFI_AP_MAX_CONNECTIONS,
				.beacon_interval = WIFI_AP_BEACON_INTERVAL,
		},
	};

	// Configure DHCP for the AP
    esp_netif_ip_info_t ap_ip_info;
    memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));

    // Parar DHCP del AP antes de cambiar IP
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(esp_netif_ap));

    // Convertir cadenas "192.168.0.1" → formato ip4
    ap_ip_info.ip.addr      = ipaddr_addr(WIFI_AP_IP);
    ap_ip_info.gw.addr      = ipaddr_addr(WIFI_AP_GATEWAY);
    ap_ip_info.netmask.addr = ipaddr_addr(WIFI_AP_NETMASK);

    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));

    // Arrancar de nuevo servidor DHCP
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));					// Start the AP DHCP server (for connecting stations e.g. your mobile device)

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));						// Setting the mode as Access Point / Station Mode
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));			// Set our configuration
	ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_AP_BANDWIDTH));		// Our default bandwidth 20 MHz
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));						// Power save set to "NONE"

}

/**
 * Configures the WiFi station settings.
 */
static void wifi_app_sta_config(void)
{
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &s_sta_config));
}

/**
 * Main task for the WiFi application
 * @param pvParameters parameter which can be passed to the task
 */
static void wifi_app_task(void *pvParameters)
{
    wifi_app_queue_message_t msg;

    // Initialize the event handler
    wifi_app_event_handler_init();

    // Initialize the TCP/IP stack and WiFi config
    wifi_app_default_wifi_init();

    // SoftAP config
    wifi_app_soft_ap_config();

    // Station config
    wifi_app_sta_config();

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    // Send first event message - start server after WiFi config
    wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);

    for (;;)
    {
        if (xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY))
        {
            switch (msg.msgID)
            {
                case WIFI_APP_MSG_START_HTTP_SERVER:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_START_HTTP_SERVER");
                    http_server_start();
                    // rgb_led_http_server_started();
                    break;

                case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER");
                    break;

                case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_STA_CONNECTED_GOT_IP");

                    // 1️⃣ Inicializa SNTP (NULL = pool.ntp.org por defecto)
                    time_service_init(NULL);

                    // 2️⃣ Espera a que se sincronice la hora (máx 10 segundos)
                    if (time_service_wait_for_sync(10000)) {
                        // Opcional, pero recomendado: zona horaria de Colombia (UTC-5)
                        time_service_set_timezone("GMT+5");
                        time_service_print_current_time();
                    } else {
                        ESP_LOGW(TAG, "No se pudo sincronizar la hora en el tiempo esperado");
                    }

                    // rgb_led_wifi_connected();
                    wifi_app_call_callback();
                    break;

                case WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT");
                    esp_wifi_disconnect();
                    break;

                case WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS");
                    // Por ahora no haces nada, pero el case debe existir
                    break;

                case WIFI_APP_MSG_STA_DISCONNECTED:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED");
                    // Aquí podrías apagar LED, limpiar flags, etc.
                    break;

                default:
                    break;
            }
        }
    }
}


BaseType_t wifi_app_send_message(wifi_app_message_e msgID)
{
	wifi_app_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(wifi_app_queue_handle, &msg, portMAX_DELAY);
}

void wifi_app_start(void)
{
	ESP_LOGI(TAG, "STARTING WIFI APPLICATION");

	// Start WiFi started LED
	// rgb_led_wifi_app_started();

	// Disable default WiFi logging messages
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// Create message queue
	wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_message_t));

	// Start the WiFi application task
	xTaskCreatePinnedToCore(&wifi_app_task, "wifi_app_task", WIFI_APP_TASK_STACK_SIZE, NULL, WIFI_APP_TASK_PRIORITY, NULL, WIFI_APP_TASK_CORE_ID);
}

wifi_config_t* wifi_app_get_wifi_config(void)
{
	return &s_sta_config;
}

void wifi_app_set_callback(wifi_connected_event_callback_t cb)
{
	s_connected_cb = cb;
}

void wifi_app_call_callback(void)
{
	if (s_connected_cb)
	{
		s_connected_cb();
	}
}

int8_t wifi_app_get_rssi(void)
{
	wifi_ap_record_t ap;
	if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
	{
		return ap.rssi;
	}
	return 0;
}