#include <stdlib.h>
#include "watchdog.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/timer.h"
#include "esp_timer.h"

////

watchdog_t g_watchdog;

void watchdog_bg_task(void* arg)
{
    while (true)
    {
        for (int i = 0; i < WATCHDOG_MAX_TASKS; i++)
        {
            size_t t0 = esp_timer_get_time();
            if (g_watchdog.feeders[i])
            {
                if (g_watchdog.feeders[i]->expected_reset_max_offset < t0)
                {
                    if (g_watchdog.feeders[i]->callback) g_watchdog.feeders[i]->callback(0);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(WATCHDOG_SLEEP_TIME_MS));
    }
}

void watchdog_init(watchdog_handle_t _wdog)
{
    for (int i = WATCHDOG_MAX_TASKS - 1; i >= 0; i--) g_watchdog.feeders[i] = 0;
    xTaskCreate((void*)watchdog_bg_task, 0, 1024, _wdog, configMAX_PRIORITIES - 1, g_watchdog.bg_task);
}
void watchdog_disconnect(watchdog_handle_t wdog)
{
    vTaskDelete(g_watchdog.bg_task);
}
void watchdog_reconnect(watchdog_handle_t wdog)
{
    xTaskCreate((void*)watchdog_bg_task, 0, 1024, wdog, configMAX_PRIORITIES - 1, g_watchdog.bg_task);
}
size_t watchdog_destroy(watchdog_handle_t wdog)
{
    vTaskDelete(g_watchdog.bg_task);
    for (int i = WATCHDOG_MAX_TASKS - 1; i >= 0; i--)
    {
        if (g_watchdog.feeders[i])
        {
            free(g_watchdog.feeders[i]);
        }
    }
    int ret_task_count = g_watchdog.running_task_count;
    g_watchdog.running_task_count = 0;
    return ret_task_count;
}

TaskHandle_t watchdog_append(watchdog_handle_t wdog, size_t delay_ms, watchdog_callback_t callback, bool is_critical, const char* name)
{
    if (g_watchdog.running_task_count == WATCHDOG_MAX_TASKS) return 0;
    TaskHandle_t feeder = malloc(sizeof(struct m_watchdog_feeder));
    feeder->callback = callback;
    feeder->delay = delay_ms * 1000;
    feeder->is_critical = is_critical;
    feeder->name = name;
    feeder->expected_reset_max_offset = feeder->delay + esp_timer_get_time();
    int i = 0;
    while (g_watchdog.feeders[i]) i++;
    g_watchdog.feeders[i] = feeder;
    g_watchdog.running_task_count++;
    return feeder;
}
bool watchdog_pop(watchdog_handle_t wdog, TaskHandle_t feeder)
{
    int i = 0;
    while (i < WATCHDOG_MAX_TASKS && (strcmp(g_watchdog.feeders[i]->name, feeder->name))) i++;
    if (i == WATCHDOG_MAX_TASKS) return false;
    g_watchdog.feeders[i] = 0;
    return true;
}

int watchdog_feed(watchdog_handle_t wdog, TaskHandle_t feeder)
{
    size_t t0 = esp_timer_get_time();
    int ret_status = WATCHDOG_TASK_STATUS_OK;
    if (feeder->expected_reset_max_offset < t0)
    {
        ret_status = WATCHDOG_TASK_STATUS_LATE;
    }
    feeder->expected_reset_max_offset = t0 + feeder->delay;
    return ret_status;
}
int watchdog_force_check(watchdog_handle_t wdog, TaskHandle_t feeder)
{
    size_t t0 = esp_timer_get_time();
    if (feeder->expected_reset_max_offset < t0)
    {
        if (feeder->callback) feeder->callback(0);
        if (feeder->is_critical) esp_restart();
        return WATCHDOG_TASK_STATUS_TIMED_OUT;
    }
    return WATCHDOG_TASK_STATUS_OK;
}
int watchdog_force_check_all(watchdog_handle_t wdog)
{
    for (int i = 0; i < WATCHDOG_MAX_TASKS; i++)
    {
        if (g_watchdog.feeders[i])
        {
            watchdog_force_starve(wdog, g_watchdog.feeders[i]);
        }
    }
    return 0;
}
int watchdog_force_starve(watchdog_handle_t wdog, TaskHandle_t feeder)
{
    if (feeder->callback) feeder->callback(0);
    if (feeder->is_critical) esp_restart();
    return 0;
}

