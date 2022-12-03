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

/* DEBUG */
void sendmsg(char* x);

////

#ifndef WATCHDOG_MAX_TASKS
#define WATCHDOG_MAX_TASKS 16
#endif

#define WATCHDOG_SLEEP_TIME_MS 1000

#define WATCHDOG_TASK_STATUS_OK 0
#define WATCHDOG_TASK_STATUS_TIMED_OUT 1
#define WATCHDOG_TASK_STATUS_LATE 2

////


typedef void(*watchdog_callback_t)(void*);

/* Deprecated
typedef struct m_watchdog_feeder {
    watchdog_callback_t callback;
    size_t delay;
    bool is_critical;
    const char* name;
    size_t expected_reset_max_offset;
} watchdog_feeder_t, *TaskHandle_t;
*/

typedef struct m_watchdog {
  /* TaskHandle_t */ TaskHandle_t feeders[WATCHDOG_MAX_TASKS];
  size_t running_task_count;
  TaskHandle_t bg_task;
} watchdog_t, *watchdog_handle_t;


////


void watchdog_bg_task                     (void* wdog);
void watchdog_init                        (watchdog_handle_t wdog, size_t delay, watchdog_callback_t callback, size_t background_task_priority);
void watchdog_disconnect                  (watchdog_handle_t wdog);
void watchdog_reconnect                   (watchdog_handle_t wdog);
size_t watchdog_destroy                   (watchdog_handle_t wdog);
bool watchdog_append  (watchdog_handle_t wdog, bool is_critical, const char* name);
bool watchdog_pop                         (watchdog_handle_t wdog, TaskHandle_t feeder);
int watchdog_feed                         (watchdog_handle_t wdog, TaskHandle_t feeder);
int watchdog_force_check                  (watchdog_handle_t wdog, TaskHandle_t feeder);
int watchdog_force_check_all              (watchdog_handle_t wdog);
int watchdog_force_starve                 (watchdog_handle_t wdog, TaskHandle_t feeder);
size_t feeder_change_delay                (watchdog_handle_t wdog, TaskHandle_t feeder, size_t delay);


