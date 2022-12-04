#include <stdlib.h>
#include "watchdog.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/timer.h"
#include "esp_timer.h"

////

watchdog_t g_watchdog; // the global watchdog

void watchdog_background_task(void* arg)
{
    while (true)
    {
        watchdog_check_all();
        vTaskDelay(pdMS_TO_TICKS(WATCHDOG_SLEEP_TIME_MS));
    }
}

void watchdog_init(size_t delay, size_t watchdog_task_stack_size, size_t watchdog_task_priority, watchdog_callback_t callback)
{
#ifndef WATCHDOG_STATIC
    g_watchdog.feeder_count = 0;
#endif
    g_watchdog.user_callback = callback;
    g_watchdog.delay = pdMS_TO_TICKS(delay);
    for (int i = WATCHDOG_MAX_TASKS - 1; i >= 0; i--) g_watchdog.feeders[i] = 0;
    xTaskCreate((void*)watchdog_background_task, 0, watchdog_task_stack_size, 0, watchdog_task_priority, g_watchdog.watchdog_task_handle);
}
void watchdog_disconnect()
{
    vTaskDelete(g_watchdog.watchdog_task_handle);
    g_watchdog.watchdog_task_handle = 0;
}
void watchdog_reconnect(size_t delay, size_t watchdog_task_stack_size, size_t watchdog_task_priority, watchdog_callback_t callback)
{
    if (g_watchdog.watchdog_task_handle)
    {
        vTaskDelete(g_watchdog.watchdog_task_handle);
    }
    xTaskCreate((void*)watchdog_background_task, 0, watchdog_task_stack_size, 0, watchdog_task_priority, g_watchdog.watchdog_task_handle);
}

#ifdef WATCHDOG_STATIC

bool watchdog_append(watchdog_uid_t feeder_uid, TaskHandle_t feeder)
{
    g_watchdog.feeders[i] = feeder;
    g_watchdog.flags[i] = false;
    return true;
}

bool watchdog_pop(watchdog_uid_t feeder_uid)
{
    g_watchdog.feeders[feeder_uid] = 0;
    return true;
}

int watchdog_feed(watchdog_uid_t feeder_uid)
{
    g_watchdog.flags[feeder_uid] = false;
    return 0;
}
int watchdog_check(TaskHandle_t feeder)
{
    if (g_watchdog.flags[i])
    {
        watchdog_starve(feeder);
    }
    g_watchdog.flags[i] = true;
    return 0;
}
int watchdog_check_all()
{
    for (int i = 0; i < WATCHDOG_MAX_TASKS; i++)
    {
        if (g_watchdog.flags[i])
        {
            watchdog_starve(g_watchdog.feeders[i]);
        }
        g_watchdog.flags[i] = true;
    }
    return 0;
}
int watchdog_starve(watchdog_uid_t feeder_uid)
{
    g_watchdog.user_callback(g_watchdog.feeders[feeder_uid]);
    return 0;
}

#else

bool watchdog_append(TaskHandle_t feeder)
{
    if (g_watchdog.feeder_count == WATCHDOG_MAX_TASKS) return false;
    int i = 0;
    while (g_watchdog.feeders[i]) i++;
    g_watchdog.feeders[i] = feeder;
    g_watchdog.flags[i] = false;
    g_watchdog.feeder_count++;
    return true;
}
bool watchdog_pop(TaskHandle_t feeder)
{
    int i = 0;
    while (i < WATCHDOG_MAX_TASKS && (g_watchdog.feeders[i] != feeder)) i++;
    if (i == WATCHDOG_MAX_TASKS) return false;
    g_watchdog.feeders[i] = 0;
    return true;
}

int watchdog_feed(TaskHandle_t feeder)
{
    int i = 0;
    while (i < WATCHDOG_MAX_TASKS && g_watchdog.feeders[i] != feeder)
    {
        i++;
    }
    if (i == WATCHDOG_MAX_TASKS)
    {
        return 1;
    }
    g_watchdog.flags[i] = false;
    return 0;
}
int watchdog_check(TaskHandle_t feeder)
{
    int i = 0;
    while (i < WATCHDOG_MAX_TASKS && g_watchdog.feeders[i] != feeder)
    {
        i++;
    }
    if (i == WATCHDOG_MAX_TASKS)
    {
        return 1;
    }
    if ((g_watchdog.feeders[i]) && g_watchdog.flags[i])
    {
        watchdog_starve(feeder);
    }
    g_watchdog.flags[i] = true;
    return 0;
}
int watchdog_check_all()
{
    for (int i = 0; i < WATCHDOG_MAX_TASKS; i++)
    {
        if ((g_watchdog.feeders[i]) && g_watchdog.flags[i])
        {
            watchdog_starve(g_watchdog.feeders[i]);
        }
        g_watchdog.flags[i] = true;
    }
    return 0;
}
int watchdog_starve(TaskHandle_t feeder)
{
    g_watchdog.user_callback(feeder);
    return 0;
}

#endif
