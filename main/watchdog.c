#include <stdlib.h>
#include "watchdog.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/timer.h"
#include "esp_timer.h"

////
void watchdog_bg_task(void* arg)
{
    watchdog_handle_t wdog = (watchdog_handle_t)arg;
    while (true)
    {
        for (int i = 0; i < WATCHDOG_MAX_TASKS; i++)
        {
            size_t t0 = esp_timer_get_time();
            if (wdog->feeders[i]->expected_reset_max_offset < t0)
            {
                if (wdog->log_pos < WATCHDOG_LOG_SIZE)
                {
                    int printlen = snprintf(wdog->log + wdog->log_pos, WATCHDOG_LOG_SIZE - wdog->log_pos, WATCHDOG_MESSAGE_LATENCY, t0, wdog->feeders[i]->name);
                    wdog->log_pos += printlen;
                }
                if (wdog->feeders[i]->callback) wdog->feeders[i]->callback(0);
                if (wdog->feeders[i]->is_critical) esp_restart();
                return;
            }
            int printlen = snprintf(wdog->log + wdog->log_pos, WATCHDOG_LOG_SIZE - wdog->log_pos, WATCHDOG_MESSAGE_FAILURE, t0, wdog->feeders[i]->name);
            wdog->log_pos += printlen;
        }
        vTaskDelay(pdMS_TO_TICKS(WATCHDOG_SLEEP_TIME_MS));
    }
}

void watchdog_init(watchdog_handle_t _wdog)
{
    _wdog->running_task_count = 0;
    _wdog->log[WATCHDOG_LOG_SIZE - 1] = 0;
    _wdog->log_pos = 0;
    for (int i = WATCHDOG_MAX_TASKS - 1; i >= 0; i--) _wdog->feeders[i] = 0;
    xTaskCreate((void*)watchdog_bg_task, 0, 1024, _wdog, configMAX_PRIORITIES - 1, _wdog->bg_task);
}
void watchdog_disconnect(watchdog_handle_t wdog)
{
    vTaskDelete(wdog->bg_task);
}
void watchdog_reconnect(watchdog_handle_t wdog)
{
    xTaskCreate((void*)watchdog_bg_task, 0, 1024, wdog, configMAX_PRIORITIES - 1, wdog->bg_task);
}
size_t watchdog_destroy(watchdog_handle_t wdog)
{
    vTaskDelete(wdog->bg_task);
    for (int i = WATCHDOG_MAX_TASKS - 1; i >= 0; i--)
    {
        if (wdog->feeders[i])
        {
            free(wdog->feeders[i]);
        }
    }
    int ret_task_count = wdog->running_task_count;
    wdog->running_task_count = 0;
    return ret_task_count;
}

watchdog_feeder_handle_t watchdog_append(watchdog_handle_t wdog, size_t delay_ms, watchdog_callback_t callback, bool is_critical, const char* name)
{
    if (wdog->running_task_count == WATCHDOG_MAX_TASKS) return 0;
    watchdog_feeder_handle_t feeder = malloc(sizeof(struct m_watchdog_feeder));
    feeder->callback = callback;
    feeder->delay = delay_ms * 1000;
    feeder->is_critical = is_critical;
    feeder->name = name;
    feeder->expected_reset_max_offset = feeder->delay + esp_timer_get_time();
    int i = 0;
    while (wdog->feeders[i]) i++;
    wdog->feeders[i] = feeder;
    wdog->running_task_count++;
    return feeder;
}
bool watchdog_pop(watchdog_handle_t wdog, watchdog_feeder_handle_t feeder)
{
    int i = 0;
    while (i < WATCHDOG_MAX_TASKS && (strcmp(wdog->feeders[i]->name, feeder->name))) i++;
    if (i == WATCHDOG_MAX_TASKS) return false;
    wdog->feeders[i] = 0;
    return true;
}

int watchdog_feed(watchdog_handle_t wdog, watchdog_feeder_handle_t feeder)
{
    size_t t0 = esp_timer_get_time();
    int ret_status = WATCHDOG_TASK_STATUS_OK;
    if (feeder->expected_reset_max_offset < t0)
    {
        ret_status = WATCHDOG_TASK_STATUS_LATE;
        if (wdog->log_pos < WATCHDOG_LOG_SIZE)
        {
            int printlen = snprintf(wdog->log + wdog->log_pos, WATCHDOG_LOG_SIZE - wdog->log_pos, WATCHDOG_MESSAGE_LATENCY, t0, feeder->name);
            wdog->log_pos += printlen;
        }
    }
    feeder->expected_reset_max_offset = t0 + feeder->delay;
    return ret_status;
}
int watchdog_force_check(watchdog_handle_t wdog, watchdog_feeder_handle_t feeder)
{
    size_t t0 = esp_timer_get_time();
    if (feeder->expected_reset_max_offset < t0)
    {
        if (wdog->log_pos < WATCHDOG_LOG_SIZE)
        {
            int printlen = snprintf(wdog->log + wdog->log_pos, WATCHDOG_LOG_SIZE - wdog->log_pos, WATCHDOG_MESSAGE_LATENCY, t0, feeder->name);
            wdog->log_pos += printlen;
        }
        if (feeder->callback) feeder->callback(0);
        if (feeder->is_critical) esp_restart();
        return WATCHDOG_TASK_STATUS_TIMED_OUT;
    }
    int printlen = snprintf(wdog->log + wdog->log_pos, WATCHDOG_LOG_SIZE - wdog->log_pos, WATCHDOG_MESSAGE_FAILURE, t0, feeder->name);
    wdog->log_pos += printlen;
    return WATCHDOG_TASK_STATUS_OK;
}
int watchdog_force_check_all(watchdog_handle_t wdog)
{
    for (int i = 0; i < WATCHDOG_MAX_TASKS; i++) if (wdog->feeders[i]) watchdog_force_starve(wdog, wdog->feeders[i]);
    return 0;
}
int watchdog_force_starve(watchdog_handle_t wdog, watchdog_feeder_handle_t feeder)
{
    if (wdog->log_pos < WATCHDOG_LOG_SIZE)
    {
        int printlen = snprintf(wdog->log + wdog->log_pos, WATCHDOG_LOG_SIZE - wdog->log_pos, WATCHDOG_MESSAGE_LATENCY, (size_t)esp_timer_get_time(), feeder->name);
        wdog->log_pos += printlen;
    }
    if (feeder->callback) feeder->callback(0);
    if (feeder->is_critical) esp_restart();
    return 0;
}
char* watchdog_get_log(watchdog_handle_t wdog)
{
    char* retlog = malloc(WATCHDOG_LOG_SIZE);
    for (int i = WATCHDOG_LOG_SIZE - 1; i >= 0; i--) retlog[i] = wdog->log[i];
    return retlog;
}

size_t feeder_change_delay(watchdog_handle_t wdog, watchdog_feeder_handle_t feeder, size_t delay)
{
    size_t return_delay = feeder->delay;
    feeder->delay = delay;
    feeder->expected_reset_max_offset = delay + esp_timer_get_time();
    return return_delay;
}

