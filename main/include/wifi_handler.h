#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <esp_wifi.h>

#if __cplusplus
extern "C" {
#endif

/* PUBLIC PROTOTYPES */
void init_wifi_station();
esp_ip4_addr_t get_gateway_ip();

#if __cplusplus
}
#endif
#endif