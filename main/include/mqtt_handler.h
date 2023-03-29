#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdint.h>

#if __cplusplus
extern "C" {
#endif

/* PUBLIC CONSTANTS */
#define MQTT_MSG_SIZE         10
#define MQTT_PAYLOAD_MAX_SIZE 9

/* PUBLIC ENUMS */
typedef enum {
  MQTT_TX_MSG_ROOM_STATUS = 1,
  MQTT_TX_MSG_DETECTION_THRESHOLD = 2
} MqttTxMessageId;

/* PUBLIC PROTOTYPES */
void init_mqtt_client();
void send_mqtt_msg(MqttTxMessageId msg_id, const char* payload, uint8_t payload_size);

#if __cplusplus
}
#endif
#endif