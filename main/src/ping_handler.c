#include <ping_handler.h>

#include <esp_log.h>
#include <lwip/inet.h>
#include <ping/ping_sock.h>
#include <proj_conf.h>
#include <wifi_handler.h>

/* PRIVATE CONSTANTS */
#define TAG "ping_handler"

/* PRIVATE PROTOTYPES */
static void handle_ping_success(esp_ping_handle_t handle, void* args);
static void handle_ping_timeout(esp_ping_handle_t handle, void* args);
static void handle_ping_end(esp_ping_handle_t handle, void* args);

/* FUNCTIONS */
void init_gateway_ping() {
  if (DEBUG_LOG_ENABLED) {
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
  }
  ESP_LOGI(TAG, "Initialize gateway ping");

  esp_ping_config_t ping_conf = ESP_PING_DEFAULT_CONFIG();
  ping_conf.interval_ms = GATEWAY_PING_INTERVAL_MS;
  ping_conf.target_addr.u_addr.ip4.addr = get_gateway_ip().addr;
  ping_conf.target_addr.type = IPADDR_TYPE_V4;
  ping_conf.count = ESP_PING_COUNT_INFINITE;

  esp_ping_callbacks_t callbacks = {
      .on_ping_success = handle_ping_success,
      .on_ping_timeout = handle_ping_timeout,
      .on_ping_end = handle_ping_end,
  };

  esp_ping_handle_t ping_handle;
  ESP_ERROR_CHECK(esp_ping_new_session(&ping_conf, &callbacks, &ping_handle));
  ESP_ERROR_CHECK(esp_ping_start(ping_handle));

  ESP_LOGI(TAG, "Started gateway ping");
}

static void handle_ping_success(esp_ping_handle_t handle, void* args) {
  uint32_t elapsed_time;
  esp_ping_get_profile(handle, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
  ESP_LOGD(TAG, "Ping time = %d", elapsed_time);
}

static void handle_ping_timeout(esp_ping_handle_t handle, void* args) {
  ip_addr_t target_addr;
  esp_ping_get_profile(handle, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
  ESP_LOGW(TAG, "Ping timeout: %s", inet_ntoa(target_addr.u_addr.ip4));
}

static void handle_ping_end(esp_ping_handle_t handle, void* args) {
  uint32_t sent;
  uint32_t received;
  uint32_t time_ms;
  esp_ping_get_profile(handle, ESP_PING_PROF_REQUEST, &sent, sizeof(sent));
  esp_ping_get_profile(handle, ESP_PING_PROF_REPLY, &received, sizeof(received));
  esp_ping_get_profile(handle, ESP_PING_PROF_DURATION, &time_ms, sizeof(time_ms));
  ESP_LOGI(TAG, "Ping packets sent = %d; received = %d; total time = %d ms", sent, received, time_ms);
}