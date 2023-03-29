#ifndef WIFI_RADAR_H
#define WIFI_RADAR_H

#if __cplusplus
extern "C" {
#endif

/* PUBLIC PROTOTYPES */
void init_wifi_radar();
void start_wifi_radar_calibration();
void stop_wifi_radar_calibration();

#if __cplusplus
}
#endif
#endif