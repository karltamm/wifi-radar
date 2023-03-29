#include <esp_event.h>
#include <esp_log.h>
#include <mqtt_handler.h>
#include <nvs_flash.h>
#include <proj_conf.h>
#include <string.h>
#include <wifi_handler.h>
#include <wifi_radar.h>

/* PRIVATE CONSTANTS */
#define TAG "main"

/* PRIVATE PROTOTYPES */
static void set_logging_settings();
static void init_storage();

/* MAIN */
void app_main(void) {
  set_logging_settings();
  init_storage();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  init_wifi_station();
  init_mqtt_client();
  init_wifi_radar();
}

/* FUNCTIONS */
static void set_logging_settings() {
  esp_log_level_set("*", ESP_LOG_INFO);
  if (DEBUG_LOG_ENABLED) {
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
  }
}

static void init_storage() {
  esp_err_t result = nvs_flash_init();
  if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }
}
