#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/hw_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "dht/dht.h"
#include "cJSON.h"

#include "connect/connect.h"
#include "fridge-sensor.h"
#include "get-sntp/get-sntp.h"

#define DHT_PIN GPIO_NUM_4
#define LED_PIN GPIO_NUM_16
#define READ_DELAY 12000 / portTICK_PERIOD_MS

QueueHandle_t SensorQueue;

void app_main()
{
    setenv("TZ", "MST7MDT,M3.2.0,M11.1.0", 1);
    tzset();

    // These block until we are connected
    wifi_init_sta();
    get_sntp_init();

    setupLED();

    SensorQueue = xQueueCreate(10, sizeof(sensor_reading));

    xTaskCreate(temperature_task, "temperature task", 2048, NULL, 3, NULL);
    xTaskCreate(process_task, "process_task", 4096, NULL, 2, NULL);
}

void temperature_task(void *arg)
{
    ESP_ERROR_CHECK(dht_init(DHT_PIN, true));

    sensor_reading reading = {
        .humidity = 0,
        .temperature = 0
    };

    while (true) {
        vTaskDelay(READ_DELAY);

        gpio_set_level(LED_PIN, 0);
       
        if (dht_read_float_data(DHT_TYPE_DHT22, 4, &(reading.humidity), &(reading.temperature)) == ESP_OK) {
            time(&reading.timestamp);
            xQueueSendToBack(SensorQueue, (void*)&reading, 500 / portTICK_PERIOD_MS);
        }

        gpio_set_level(LED_PIN, 1);
    }

}

void process_task(void *arg)
{
    u_int8_t count;
    sensor_reading reading = {
        .humidity = 0,
        .temperature = 0,
        .timestamp = 0
    };
    cJSON *readings = NULL;
    cJSON *item = NULL;
    cJSON *humidity = NULL;
    cJSON *temperature = NULL;
    cJSON *timestamp = NULL;
    char *json = NULL;

    esp_http_client_config_t config = {
        .url = CONFIG_SENSOR_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 3000,
    };

    while (true) {
        count = 0;
        reading.humidity = 0;
        reading.temperature = 0;
        reading.timestamp = 0;
        readings = cJSON_CreateArray();

        for (u_int8_t i = 0;  i < 5; i++) {
            if (xQueueReceive(SensorQueue, &reading, READ_DELAY) == pdPASS) {
                item = cJSON_CreateObject();
                cJSON_AddItemToArray(readings, item);

                humidity = cJSON_CreateNumber(reading.humidity);
                cJSON_AddItemToObject(item, "h", humidity);
                
                temperature = cJSON_CreateNumber(reading.temperature);
                cJSON_AddItemToObject(item, "t", temperature);
                
                timestamp = cJSON_CreateNumber(reading.timestamp);
                cJSON_AddItemToObject(item, "z", timestamp);
                
                count++;
            }
        }

        printf("%ld\n", time(NULL));
        printf("Got %d items\n", count);

        if (count > 0) {
            json = cJSON_Print(readings);

            for (u_int8_t retry = 0; retry < 3; retry++) {
                esp_http_client_handle_t client = esp_http_client_init(&config);
                esp_http_client_set_header(client, "Content-Type", "application/json");
                esp_http_client_set_post_field(client, json, strlen(json));

                esp_err_t result = esp_http_client_perform(client);
                esp_http_client_cleanup(client);

                if (result == ESP_OK) {
                    // Worked the first time
                    break;
                }
            }

            free(json);
        }

        cJSON_Delete(readings);
        
        printf("Free Heap: %d\n", esp_get_free_heap_size());
        printf("Waiting: %d, Free: %d\n", uxQueueMessagesWaiting(SensorQueue), uxQueueSpacesAvailable(SensorQueue));
    }

    vTaskDelete(NULL);
}

static inline void setupLED()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LED_PIN,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    /// LED is active low (?)
    gpio_set_level(LED_PIN, 1);
}
