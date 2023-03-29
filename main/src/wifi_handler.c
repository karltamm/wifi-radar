#include <wifi_handler.h>

#include <esp_event.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <lwip/err.h>
#include <lwip/inet.h>
#include <lwip/ip_addr.h>
#include <lwip/sys.h>
#include <proj_conf.h>
#include <string.h>

/* PRIVATE CONSTANTS */
#define TAG "wifi_handler"

#define WIFI_CONNECTED_FLAG BIT0

/* GLOBAL VARIABLES */
static EventGroupHandle_t g_wifi_event_group = NULL;
static esp_ip4_addr_t g_gateway_ip = {0};

/* PRIVATE PROTOTYPES */
static void handle_wifi_events(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void set_gateway_ip(esp_ip4_addr_t* ip_address);

/* FUNCTIONS */
void init_wifi_station() {
  ESP_LOGI(TAG, "Initializing Wifi station");

  g_wifi_event_group = xEventGroupCreate();
  esp_event_handler_instance_t any_id_event_handler_instance;
  esp_event_handler_instance_t got_ip_event_handler_instance;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      handle_wifi_events,
                                                      NULL,
                                                      &any_id_event_handler_instance));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      handle_wifi_events,
                                                      NULL,
                                                      &got_ip_event_handler_instance));

  ESP_ERROR_CHECK(esp_netif_init());
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t init_conf = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&init_conf));

  wifi_config_t wifi_conf = {
      .sta = {
          .ssid = WIFI_AP_SSID,
          .password = WIFI_AP_PASSWORD,
          .threshold.authmode = WIFI_AUTH_WPA2_PSK,
      },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_conf));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
  ESP_ERROR_CHECK(esp_wifi_start());

  EventBits_t event_bits = xEventGroupWaitBits(g_wifi_event_group, WIFI_CONNECTED_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
  if (event_bits & WIFI_CONNECTED_FLAG) {
    ESP_LOGI(TAG, "Connected to Wifi AP. SSID: %d. Password: %s", WIFI_AP_SSID, WIFI_AP_PASSWORD);
    xEventGroupClearBits(g_wifi_event_group, WIFI_CONNECTED_FLAG);
  } else {
    ESP_LOGE("Failed to connect to Wifi AP. SSID: %d. Password: %s", WIFI_AP_SSID, WIFI_AP_PASSWORD);
  }
  ESP_LOGI(TAG, "Wifi station is initialized");
}

esp_ip4_addr_t get_gateway_ip() {
  return g_gateway_ip;
}

static void handle_wifi_events(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
      case WIFI_EVENT_STA_START:
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;

      case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGW(TAG, "Wifi disconnected");
        ESP_LOGI(TAG, "Reconnecting to Wifi AP");
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    }
  } else if (event_base == IP_EVENT) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
      ESP_LOGI(TAG, "Wifi station got IP: " IPSTR, IP2STR(&event->ip_info.ip));
      set_gateway_ip(&event->ip_info.gw);
      xEventGroupSetBits(g_wifi_event_group, WIFI_CONNECTED_FLAG);
    }
  }
}

static void set_gateway_ip(esp_ip4_addr_t* ip_address) {
  memcpy(&g_gateway_ip, ip_address, sizeof(esp_ip4_addr_t));
  ESP_LOGI(TAG, "Set Wifi AP IP:" IPSTR, IP2STR(&g_gateway_ip));
}
