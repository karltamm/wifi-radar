# WiFi Radar
WiFi Radar is a project that utilizes WiFi CSI (Channel State Information) to detect human presence in a room. The project is based on an ESP32-C3 development board that pings a WiFi router and analyzes received CSI from the router in response. The analysis of the CSI helps to detect the subtle changes in the WiFi signal caused by the human body movement.

The device sends the room status over MQTT that allows for easy integration with other systems. It can also be controlled remotely via MQTT to use the training mode that enables the user to set a more accurate threshold for an empty room. By using this mode, the device can differentiate between normal background WiFi signals and signals caused by human movement more accurately, resulting in more reliable detection.
