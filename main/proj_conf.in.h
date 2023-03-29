#ifndef CONF_H
#define CONF_H

#define DEVICE_ID "1"

#define DEBUG_LOG_ENABLED 0

#define WIFI_AP_SSID     "@WIFI_AP_SSID@"      // Replaced by CMake using env var
#define WIFI_AP_PASSWORD "@WIFI_AP_PASSWORD@"  // Replaced by CMake using env var

#define GATEWAY_PING_INTERVAL_MS 10

#define MQTT_RX_TOPIC "radar/" DEVICE_ID "/to"
#define MQTT_TX_TOPIC "radar/" DEVICE_ID "/from"

#define MQTT_HOST      "193.40.245.72"
#define MQTT_PORT      1883
#define MQTT_USERNAME  "test"
#define MQTT_PASSWORD  "test"
#define MQTT_CLIENT_ID "wifi-radar-" DEVICE_ID

#endif