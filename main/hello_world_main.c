#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/uart_select.h"
#include "driver/i2c.h"
#include "driver/timer.h"
#include "esp_timer.h"

#include "watchdog.h"

#define BREAK_CHARACTER '\n'
#define START_INTERVAL 500
#define UART2_BAUDRATE 9600

watchdog_t wdog;

static QueueHandle_t interval_queue;
static esp_timer_handle_t timer;

void config_uart2()
{
    //
    uart_set_pin(UART_NUM_2, GPIO_NUM_1, GPIO_NUM_3, GPIO_NUM_22, GPIO_NUM_23);
    //
    uart_driver_install(UART_NUM_2, 1024, 1024, 10, 0, 0);
    //
    uart_set_baudrate(UART_NUM_2, UART2_BAUDRATE);
    uart_set_word_length(UART_NUM_2, UART_DATA_8_BITS);
    uart_set_parity(UART_NUM_2, UART_PARITY_DISABLE);
    uart_set_stop_bits(UART_NUM_2, UART_STOP_BITS_1);
    uart_set_hw_flow_ctrl(UART_NUM_2, UART_HW_FLOWCTRL_DISABLE, 122);
    uart_set_mode(UART_NUM_2, UART_MODE_UART);
}

bool is_timed_out = false;
void TimerCallback(void* _targs)
{
    is_timed_out = true;
}

unsigned mini(unsigned a, unsigned b)
{
    return (((a) < (b)) ? (a) : (b));
}

void t1c(void* _args)
{
    uart_write_bytes(UART_NUM_2, "Write Task Callback Called\n", 26);
}

void t2c(void* _args)
{
    if (_args)
    {
        uart_write_bytes(UART_NUM_2, "SHOULD RECEIVE A MESSAGE:\n", 26);
        uart_write_bytes(UART_NUM_2, _args, strlen(_args));
    }
    uart_write_bytes(UART_NUM_2, "Read Task Callback Called\n", 25);
}

void WriteTask(void* _targs)
{
    static unsigned led_interval = START_INTERVAL * 1000;
    static unsigned char msg_count;
    static bool toggle = false;
    static char num_buf[13];
    static unsigned num_len;
    //
    TaskHandle_t fhandle = watchdog_append(&wdog, 2500, t1c, false, "WriteTask");
    //
    num_buf[12] = '\n';
    while (true)
    {
        msg_count = uxQueueMessagesWaiting(interval_queue);
        if (msg_count)
        {
            while (msg_count--) xQueueReceive(interval_queue, &led_interval, 0);
            num_len = mini(snprintf(num_buf, 12, "%u", led_interval), 12);
            num_buf[num_len] = '\n';
            led_interval *= 1000; // millis to micros
            esp_timer_stop(timer);
            esp_timer_start_periodic(timer, led_interval);
            uart_write_bytes(UART_NUM_2, "Interval Changed Succesfully To: ", 33);
            uart_write_bytes(UART_NUM_2, num_buf, num_len + 1);
        }
        if (is_timed_out)
        {
            gpio_set_level(GPIO_NUM_4, (toggle) ? 1 : 0);
            toggle = !toggle;
            is_timed_out = false;
        }
        watchdog_feed(&wdog, fhandle);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void ReadTask(void* _targs)
{
    static char buf;
    static size_t uart_buf_len = 0;
    static unsigned sent_interval = 0;
    //
    TaskHandle_t fhandle = watchdog_append(&wdog, 100, t2c, true, "ReadTask");
    //
    while (true)
    {
        uart_get_buffered_data_len(UART_NUM_2, &uart_buf_len);
        while (uart_buf_len--)
        {
            uart_read_bytes(UART_NUM_2, &buf, 1, 1);
            if (buf == BREAK_CHARACTER)
            {
                xQueueSendToBack(interval_queue, &sent_interval, 1);
                sent_interval = 0;
            }
            else sent_interval = 10 * sent_interval + (buf - '0');
        }
        watchdog_feed(&wdog, fhandle);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* DEBUG */
void sendmsg(char* x)
{
    uart_write_bytes(UART_NUM_2, x, strlen(x));
}

///*
void app_main(void)
{
    // pre-setup
    esp_timer_early_init();
    esp_timer_init();
    esp_timer_create_args_t timer_config = {
        .callback=&TimerCallback,
        .arg=0,
        .dispatch_method=ESP_TIMER_TASK,
        .name="Timer1",
        .skip_unhandled_events=0
    };
    // setup
    vTaskDelay(pdMS_TO_TICKS(100));
    watchdog_init(&wdog);
    config_uart2();
    interval_queue = xQueueCreate(64, sizeof(unsigned));
    esp_timer_create(&timer_config, &timer);
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    // task config
    xTaskCreatePinnedToCore(ReadTask , "ReadTask" , 2048, 0, 1, 0, 0);
    xTaskCreatePinnedToCore(WriteTask, "WriteTask", 2048, 0, 2, 0, 1);
    esp_timer_start_periodic(timer, START_INTERVAL * 1000);
    // start
}
// */