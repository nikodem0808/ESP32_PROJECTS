#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/timer.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"


////
#define WATCHDOG_MAX_TASKS 16
#define WATCHDOG_LOG_SIZE 4096
#define WATCHDOG_SLEEP_TIME_MS 1000

#define WATCHDOG_TASK_STATUS_OK 1
#define WATCHDOG_TASK_STATUS_TIMED_OUT 1
#define WATCHDOG_TASK_STATUS_LATE 2

// the watchdog message should be a formatted string with (%z, %s) being (size_t absolute_time, const char* task_name)
#define WATCHDOG_MESSAGE_SUCCESS "%u : %s checked in on time\n"
#define WATCHDOG_MESSAGE_FAILURE "%u %s failed to check in\n"
#define WATCHDOG_MESSAGE_LATENCY "%u %s checked in late\n"
//#define WATCHDOG_MESSAGE_SIZE strlen(WATCHDOG_MESSAGE)
////
typedef struct m_watchdog_feeder watchdog_feeder_t, *watchdog_feeder_handle_t;
typedef void(*watchdog_callback_t)(void*);
typedef struct m_watchdog watchdog_t, *watchdog_handle_t;
////
void watchdog_bg_task(void* wdog);
void watchdog_init(watchdog_handle_t wdog);
void watchdog_disconnect(watchdog_handle_t wdog);
void watchdog_reconnect(watchdog_handle_t wdog);
size_t watchdog_destroy(watchdog_handle_t wdog);
watchdog_feeder_handle_t watchdog_append(watchdog_handle_t wdog, size_t delay, watchdog_callback_t callback, bool is_critical, const char* name);
bool watchdog_pop(watchdog_handle_t wdog, watchdog_feeder_handle_t feeder);
int watchdog_feed(watchdog_handle_t wdog, watchdog_feeder_handle_t feeder);
int watchdog_force_check(watchdog_handle_t wdog, watchdog_feeder_handle_t feeder);
int watchdog_force_check(watchdog_handle_t wdog, watchdog_feeder_handle_t feeder);
int watchdog_force_starve(watchdog_handle_t wdog, watchdog_feeder_handle_t feeder);
char* watchdog_get_log(watchdog_handle_t wdog);

size_t feeder_change_delay(watchdog_handle_t wdog, watchdog_feeder_handle_t feeder, size_t delay);


