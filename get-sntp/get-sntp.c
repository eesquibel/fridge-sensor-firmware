#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event.h"
#include "lwip/apps/sntp.h"

#include "get-sntp.h"

static EventGroupHandle_t get_sntp_event_group;

bool get_sntp_init(void)
{
    ESP_LOGI("ntp", "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    get_sntp_event_group = xEventGroupCreate();

    xTaskCreate(get_sntp_wait, "get_sntp_wait", 2048, NULL, 3, NULL);
    EventBits_t status = xEventGroupWaitBits(get_sntp_event_group, BIT0, pdFALSE, pdFALSE, 60000 / portTICK_PERIOD_MS);

    vEventGroupDelete(get_sntp_event_group);

    if (status && BIT0) {
        return true;
    } else {
        return false;
    }
}


void get_sntp_wait(void *arg)
{
    u_int8_t retry = 0;
    time_t t;
    struct tm tm;
    char buf[50];

    while (++retry < 60) {
        time(&t);
        localtime_r(&t, &tm);
        
        if (tm.tm_year > 70) {
            ESP_LOGI("ntp", "Got date: %s", asctime_r(&tm, buf));
            xEventGroupSetBits(get_sntp_event_group, BIT0);
            break;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}