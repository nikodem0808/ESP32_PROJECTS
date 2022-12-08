#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
#warning "To define the max amount of tasks the watchdog should track, define the WATCHDOG_MAX_TASKS macro"
#define WATCHDOG_MAX_TASKS 16
#endif

////



typedef void(*watchdog_callback_t)(TaskHandle_t);
#ifdef WATCHDOG_STATIC
typedef size_t watchdog_uid_t;
#else
// tu bylo puste nwm o co chodzilo ale jak bedzie error to sie okaze
#endif




////

void watchdog_background_task             (void* arg);
void watchdog_init                        (size_t delay, size_t watchdog_stack_size, size_t watchdog_task_priority, watchdog_callback_t callback);
void watchdog_disconnect                  ();
void watchdog_reconnect                   ();
int watchdog_check_all                    ();

#ifdef WATCHDOG_STATIC

bool watchdog_append                      (watchdog_uid_t feeder_uid, TaskHandle_t feeder);
bool watchdog_pop                         (watchdog_uid_t feeder_uid);
int watchdog_feed                         (watchdog_uid_t feeder_uid);
int watchdog_check                        (watchdog_uid_t feeder_uid);
int watchdog_starve                       (watchdog_uid_t feeder_uid);

#else

bool watchdog_append                      (TaskHandle_t feeder);
bool watchdog_pop                         (TaskHandle_t feeder);
int watchdog_feed                         (TaskHandle_t feeder);
int watchdog_check                        (TaskHandle_t feeder);
int watchdog_starve                       (TaskHandle_t feeder);

#endif

// include guard end
#endif
