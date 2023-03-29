idf_component_register(
    SRCS
    "src/main.c"
    "src/wifi_radar.c"
    "src/wifi_handler.c"
    "src/mqtt_handler.c"
    "src/ping_handler.c"
    INCLUDE_DIRS
    "."
    "include"
)

# LIBRARIES
add_library(esp-csi-lib STATIC IMPORTED)
set_target_properties(esp-csi-lib PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../libs/esp-csi/libesp-csi.a")
target_include_directories(esp-csi-lib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../libs/esp-csi")
target_link_libraries(${COMPONENT_LIB} esp-csi-lib)

# CONFIGURED HEADER FILES
if(DEFINED ENV{WIFI_RADAR_AP_SSID})
    set(WIFI_AP_SSID $ENV{WIFI_RADAR_AP_SSID})
else()
    message(FATAL_ERROR "Wifi AP SSID env var is not set")
endif()

if(DEFINED ENV{WIFI_RADAR_AP_PASSWORD})
    set(WIFI_AP_PASSWORD $ENV{WIFI_RADAR_AP_PASSWORD})
else()
    message(FATAL_ERROR "Wifi AP password env var is not set")
endif()

set(CONF_FILE_DIR "${PROJECT_DIR}/build/config")
configure_file(proj_conf.in.h ${CONF_FILE_DIR}/proj_conf.h)
target_include_directories(${COMPONENT_LIB} PUBLIC ${CONF_FILE_DIR})