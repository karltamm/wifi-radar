#include <mqtt_handler.h>

#include <esp_event.h>
#include <esp_log.h>
#include <mqtt_client.h>
#include <proj_conf.h>
#include <wifi_radar.h>

/* PRIVATE CONSTANTS */
#define TAG "mqtt_handler"

/* PRIVATE ENUMS */
typedef enum {
  MQTT_RX_MSG_START_CALIBRATION = 1,
  MQTT_RX_MSG_STOP_CALIBRATION = 2,
} MqttRxMessageId;

/* GLOBAL VARIABLES */
static esp_mqtt_client_handle_t g_mqtt_client = NULL;

/* PRIVATE PROTOTYPES */
static void handle_mqtt_events(void* args, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void log_mqtt_error_if_nonzero(const char* message, int error_code);
static void handle_rx_data(esp_mqtt_event_handle_t event);

/* FUNCTIONS */
void init_mqtt_client() {
  if (DEBUG_LOG_ENABLED) {
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
  }
  ESP_LOGI(TAG, "Initialize MQTT client");
  esp_mqtt_client_config_t mqtt_conf = {
      .host = MQTT_HOST,
      .port = MQTT_PORT,
      .username = MQTT_USERNAME,
      .password = MQTT_PASSWORD,
      .client_id = MQTT_CLIENT_ID,
      .disable_auto_reconnect = false,
  };
  g_mqtt_client = esp_mqtt_client_init(&mqtt_conf);
  ESP_ERROR_CHECK(esp_mqtt_client_register_event(g_mqtt_client, ESP_EVENT_ANY_ID, handle_mqtt_events, NULL));
  ESP_ERROR_CHECK(esp_mqtt_client_start(g_mqtt_client));
  ESP_LOGI(TAG, "Started MQTT client");
}

void send_mqtt_msg(MqttTxMessageId msg_id, const char* payload, uint8_t payload_size) {
  if (!g_mqtt_client) {
    ESP_LOGW(TAG, "Can't send MQTT message cause client doesn't exist");
    return;
  }

  if (payload_size > MQTT_PAYLOAD_MAX_SIZE) {
    ESP_LOGW(TAG, " MQTT message payload size is bigger than expected");
    return;
  }

  char buffer[MQTT_MSG_SIZE] = {0};
  buffer[0] = msg_id;
  memcpy(buffer + 1, payload, payload_size);

  if (esp_mqtt_client_publish(g_mqtt_client, MQTT_TX_TOPIC, buffer, MQTT_MSG_SIZE, 0, 0) < 0) {
    ESP_LOGW(TAG, "Couldn't publish MQTT message");
  }
}

static void handle_mqtt_events(void* args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  esp_mqtt_event_handle_t event = event_data;
  esp_mqtt_client_handle_t client = event->client;
  switch (event_id) {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "MQTT connected");
      esp_mqtt_client_subscribe(client, MQTT_RX_TOPIC, 0);
      break;

    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "MQTT disconnected");
      break;

    case MQTT_EVENT_SUBSCRIBED:
      ESP_LOGI(TAG, "MQTT topic subscribed");
      break;

    case MQTT_EVENT_UNSUBSCRIBED:
      ESP_LOGI(TAG, "MQTT topic unsubscribed");
      break;

    case MQTT_EVENT_DATA:
      handle_rx_data(event);
      break;

    case MQTT_EVENT_ERROR:
      ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
      if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
        log_mqtt_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
        log_mqtt_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
        log_mqtt_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
        ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
      }
      break;
    default:
      break;
  }
}

static void log_mqtt_error_if_nonzero(const char* message, int error_code) {
  if (error_code != 0) {
    ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
  }
}

static void handle_rx_data(esp_mqtt_event_handle_t event) {
  if (!event->data_len) {
    ESP_LOGW(TAG, "Received empty MQTT message: %d", event->data_len);
    return;
  }
  const char rx_message_id = event->data[0];
  switch (rx_message_id) {
    case MQTT_RX_MSG_START_CALIBRATION:
      start_wifi_radar_calibration();
      break;
    case MQTT_RX_MSG_STOP_CALIBRATION:
      stop_wifi_radar_calibration();
      break;
    default:
      ESP_LOGW(TAG, "Received unexpected MQTT message: %.*s; Message ID = %d", event->data_len, event->data, rx_message_id);
      break;
  }
}