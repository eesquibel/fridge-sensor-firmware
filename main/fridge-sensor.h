#include <time.h>

static inline void setupLED();

void temperature_task(void *arg);

void process_task(void *arg);

void update_clock(void *arg);

typedef struct sensor_reading {
    float humidity;
    float temperature;
    time_t timestamp;
} sensor_reading;
