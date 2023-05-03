#include <wifi_radar.h>

#include <esp_log.h>
#include <esp_radar.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>
#include <mqtt_handler.h>
#include <nvs.h>
#include <ping_handler.h>
#include <proj_conf.h>
#include <string.h>

/* PRIVATE CONSTANTS */
#define TAG "wifi_radar"

#define NVS_NAMESPACE "wifi_radar"
#define THRESHOLD_KEY "threshold"

#define RADAR_INFO_QUEUE_SIZE              100
#define TASK_PROCESS_RADAR_DATA_STACK_SIZE 4096
#define TASK_SEND_ROOM_STATUS_STACK_SIZE   4096
#define TASK_SEND_ROOM_STATUS_INTERVAL_MS  100

#define NEEDED_MEASUREMENTS_COUNT 10
#define NEEDED_DETECTIONS_COUNT   2
#define DETECTION_TIMEOUT_MS      3000

/* PRIVATE ENUMS */
typedef enum {
  ROOM_UNDEFINED = 0x00,
  NO_MOVEMENT = 0x01,
  MOVEMENT_DETECTED = 0x02,
  ROOM_CALIBRATION_ACTIVE = 0x03,
} RoomStatus;

/* GLOBAL VARIABLES */
static wifi_radar_info_t g_detection_threshold = {0};
static bool g_calibration_in_progress = false;

static bool g_radar_initialized = false;

static xQueueHandle g_radar_info_queue = NULL;
static TimerHandle_t g_detection_timeout_timer = NULL;

static wifi_radar_info_t g_measurements[NEEDED_MEASUREMENTS_COUNT] = {0};
static bool g_movement_detected = false;
static uint32_t g_measurements_count = 0;

static nvs_handle_t g_nvs_handle = 0;

/* PRIVATE PROTOTYPES */
static void configure_logging();
static void detect_presence(const wifi_radar_info_t* radar_info);
static void send_room_status(void* arg);
static void wifi_radar_callback(const wifi_radar_info_t* info, void* ctx);
static void process_radar_data();
static void detection_timeout_callback(TimerHandle_t timer);
static void load_threshold();
static void save_threshold();

/* FUNCTIONS */
void init_wifi_radar() {
  configure_logging();
  ESP_LOGI(TAG, "Initializing Wifi radar");

  ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &g_nvs_handle));
  load_threshold();

  g_radar_info_queue = xQueueCreate(RADAR_INFO_QUEUE_SIZE, sizeof(wifi_radar_info_t*));
  g_detection_timeout_timer = xTimerCreate("detection_timer", pdMS_TO_TICKS(DETECTION_TIMEOUT_MS), pdFALSE, 0, detection_timeout_callback);
  xTaskCreate(process_radar_data, "process_radar_data", TASK_PROCESS_RADAR_DATA_STACK_SIZE, NULL, 0, NULL);
  xTaskCreate(send_room_status, "send_room_status", TASK_SEND_ROOM_STATUS_STACK_SIZE, NULL, 0, NULL);

  init_gateway_ping();

  wifi_radar_config_t conf = {
      .filter_mac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},  // No filtering based on MAC address
      .wifi_radar_cb = wifi_radar_callback,
  };
  ESP_ERROR_CHECK(esp_radar_init());
  ESP_ERROR_CHECK(esp_radar_set_config(&conf));
  ESP_ERROR_CHECK(esp_radar_start());

  ESP_LOGI(TAG, "Started Wifi radar");
  g_radar_initialized = true;
}

void start_wifi_radar_calibration() {
  if (!g_radar_initialized) {
    ESP_LOGW(TAG, "Can't start calibration cause radar is not init");
  }

  if (g_calibration_in_progress) {
    ESP_LOGW(TAG, "Previous Wifi radar calibration ongoing");
    return;
  }

  g_calibration_in_progress = true;
  esp_radar_train_remove();  // Remove previous calibration
  esp_radar_train_start();
  ESP_LOGI(TAG, "Started Wifi radar calibration");
}

void stop_wifi_radar_calibration() {
  if (!g_calibration_in_progress) {
    ESP_LOGW(TAG, "Can't stop Wifi radar calibration cause it hasn't started");
    return;
  }

  esp_radar_train_stop(&g_detection_threshold.waveform_jitter,
                       &g_detection_threshold.waveform_wander);

  g_detection_threshold.waveform_jitter *= 1.1;
  g_detection_threshold.waveform_wander *= 1.1;

  save_threshold();
  g_calibration_in_progress = false;
  ESP_LOGI(TAG, "Stopped Wifi radar calibration");
}

static void configure_logging() {
  if (DEBUG_LOG_ENABLED) {
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
  } else {
    esp_log_level_set("esp_radar", ESP_LOG_WARN);
  }
}

static void wifi_radar_callback(const wifi_radar_info_t* info, void* ctx) {
  wifi_radar_info_t* radar_info = malloc(sizeof(wifi_radar_info_t));
  memcpy(radar_info, info, sizeof(wifi_radar_info_t));

  if (xQueueSend(g_radar_info_queue, &radar_info, 0) == pdFALSE) {
    ESP_LOGW(TAG, "Radar info queue is full");
    free(radar_info);
  }
}

static void process_radar_data() {
  wifi_radar_info_t* radar_info;
  while (xQueueReceive(g_radar_info_queue, &radar_info, portMAX_DELAY)) {
    detect_presence(radar_info);

    free(radar_info);
  }
}

static void detect_presence(const wifi_radar_info_t* info) {
  if (g_calibration_in_progress) {
    return;
  }

  if (g_detection_threshold.waveform_jitter == 0) {
    return;
  }

  g_measurements[g_measurements_count++ % NEEDED_MEASUREMENTS_COUNT] = *info;
  if (g_measurements_count < NEEDED_MEASUREMENTS_COUNT) {
    return;
  }

  uint8_t motion_detection_count = 0;
  for (int i = 0; i < NEEDED_MEASUREMENTS_COUNT; i++) {
    if (g_measurements[i].waveform_jitter > g_detection_threshold.waveform_jitter) {
      motion_detection_count++;
    }
  }

  if (motion_detection_count < NEEDED_DETECTIONS_COUNT) {
    // Currently no movement detected
    if (g_movement_detected) {
      // Movement was previously detected
      if (!xTimerIsTimerActive(g_detection_timeout_timer)) {
        /* If movement is still not detected after specified time,
        then set detection value to false */
        xTimerStart(g_detection_timeout_timer, 0);
      }
    }
  } else {
    xTimerStop(g_detection_timeout_timer, 0);
    g_movement_detected = true;
  }
}

static void send_room_status(void* arg) {
  char room_status = 0;

  while (true) {
    if (g_calibration_in_progress) {
      room_status = ROOM_CALIBRATION_ACTIVE;
    } else {
      if (g_detection_threshold.waveform_jitter == 0) {
        room_status = ROOM_UNDEFINED;
      } else {
        room_status = g_movement_detected ? MOVEMENT_DETECTED : NO_MOVEMENT;
      }
    }
    send_mqtt_msg(MQTT_TX_MSG_ROOM_STATUS, &room_status, 1);
    vTaskDelay(pdMS_TO_TICKS(TASK_SEND_ROOM_STATUS_INTERVAL_MS));
  }
}

static void detection_timeout_callback(TimerHandle_t timer) {
  g_movement_detected = false;
}

static void load_threshold() {
  size_t bytes_count;
  uint8_t bytes[sizeof(g_detection_threshold.waveform_jitter)];

  esp_err_t res = nvs_get_blob(g_nvs_handle, THRESHOLD_KEY, bytes, &bytes_count);
  if (res == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGW(TAG, "Threshold data not found in NVS");
    return;
  } else if (res != ESP_OK) {
    abort();
  }

  memcpy(&g_detection_threshold.waveform_jitter, bytes, sizeof(g_detection_threshold.waveform_jitter));
}

static void save_threshold() {
  size_t bytes_count = sizeof(g_detection_threshold.waveform_jitter);
  uint8_t bytes[bytes_count];
  memcpy(bytes, &g_detection_threshold.waveform_jitter, bytes_count);
  ESP_ERROR_CHECK(nvs_set_blob(g_nvs_handle, THRESHOLD_KEY, bytes, bytes_count));
}