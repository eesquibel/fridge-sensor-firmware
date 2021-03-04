# ESP8266 Fridge Sensor using DHT

An ESP8266 FreeRTOS project for using a DHT22 sensor. Connects to WiFi, pulls time from NTP, and POSTs sensor data to the supplied URL.

Run `idf.py menuconfig` to set the WiFi network and password, and specify the URL to POST the sensor data to.

Connect the DHT22 to GPIO 4, or edit the `DHT_PIN` #define.

Reading are taken every 2 minutes, defined by `#define READ_DELAY` (in milliseconds).

