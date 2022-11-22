/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <time.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/uart_select.h"
#include "driver/i2c.h"
//#include "semphr.h"
//#define uart_write_bytes(AX,BX,CX) printf(BX)
//unsigned readPin = 0, writePin = 0;

#define BUFSIZE 4096

static char buf[BUFSIZE];
static unsigned pos = 0, len = 0;
static size_t uart_buf_len = 0;
static unsigned hadEnter = 0;

static int input_interval = 0;

static char num_buf[13];

static unsigned const UART2_BAUDRATE = 9600;

static QueueHandle_t uart_queue;
static TaskHandle_t write_task;


void tos(unsigned num)
{
    num_buf[12] = '\n';
    for (int i = 11; i >= 0; i--)
    {
        num_buf[i] = (num % 10) + '0';
        num /= 10;
    }
}

static SemaphoreHandle_t mutex;

void config_uart2()
{
    //
    uart_set_pin(UART_NUM_2, GPIO_NUM_1, GPIO_NUM_3, GPIO_NUM_22, GPIO_NUM_23);
    //
    uart_driver_install(UART_NUM_2, 1024, 1024, 10, 0 /*&uart_queue*/, 0);
    //
    uart_set_baudrate(UART_NUM_2, UART2_BAUDRATE);
    uart_set_word_length(UART_NUM_2, UART_DATA_8_BITS);
    uart_set_parity(UART_NUM_2, UART_PARITY_DISABLE);
    uart_set_stop_bits(UART_NUM_2, UART_STOP_BITS_1);
    uart_set_hw_flow_ctrl(UART_NUM_2, UART_HW_FLOWCTRL_DISABLE, 122);
    uart_set_mode(UART_NUM_2, UART_MODE_UART);
}

void WriteTask(void* _targs)
{
    static unsigned led_interval = pdMS_TO_TICKS(500);
    vTaskDelay(pdMS_TO_TICKS(10));
    while (true)
    {
        if (xSemaphoreTake(mutex, 0))
        {
            if (input_interval)
            {
                led_interval = pdMS_TO_TICKS(input_interval);
                tos(input_interval);
                input_interval = 0;
                uart_write_bytes(UART_NUM_2, "Interval Changed Succesfully To: ", 33);
                uart_write_bytes(UART_NUM_2, num_buf, 13);
            }
            xSemaphoreGive(mutex);
        }
        gpio_set_level(GPIO_NUM_4, 1);
        vTaskDelay(led_interval);
        gpio_set_level(GPIO_NUM_4, 0);
        if (xSemaphoreTake(mutex, 0))
        {
            if (input_interval)
            {
                led_interval = pdMS_TO_TICKS(input_interval);
                tos(input_interval);
                input_interval = 0;
                uart_write_bytes(UART_NUM_2, "Interval Changed Succesfully To: ", 33);
                uart_write_bytes(UART_NUM_2, num_buf, 13);
            }
            xSemaphoreGive(mutex);
        }
        vTaskDelay(led_interval);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

unsigned to_int(void)
{
  unsigned r = 0;
  while (len--)
  {
    if (buf[pos] == '\n')
    {
        buf[pos] = '0';
        break;
    }
    r = r * 10 + (unsigned)(buf[pos] - '0');
    pos = (pos + 1) % BUFSIZE;
  }
  vTaskDelete(write_task);
  xTaskCreate(WriteTask, "WriteTask", 2048, 0, 1, &write_task);
  return r;
}
///*
void app_main(void)
{
    // setup
    config_uart2();
    mutex = xSemaphoreCreateMutex();
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    // task config
    xTaskCreate(WriteTask, "WriteTask", 2048, 0, 1, &write_task);
    // start
    while (true)
    {
        //uart_write_bytes(UART_NUM_2, "Hello Mum!\n", 11);
        //continue;
        // break */
        if ((hadEnter) && xSemaphoreTake(mutex, 0))
        {
            input_interval = to_int();
            hadEnter--;
            xSemaphoreGive(mutex);
        }
        uart_get_buffered_data_len(UART_NUM_2, &uart_buf_len);
        uart_buf_len = ((uart_buf_len + len > BUFSIZE) ? (BUFSIZE - len) : uart_buf_len);
        while (uart_buf_len--)
        {
            uart_read_bytes(UART_NUM_2, buf + ((pos + len) % BUFSIZE), 1, 1);
            len++;
            if (buf[((pos + len - 1) % BUFSIZE)] == '\n')
            {
                if (xSemaphoreTake(mutex, 0))
                {
                    input_interval = to_int();
                    xSemaphoreGive(mutex);
                }
                else
                {
                    hadEnter++;
                }
            }
            if (len == BUFSIZE)
            {
                xSemaphoreTake(mutex, portMAX_DELAY);
                input_interval = to_int();
                xSemaphoreGive(mutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
// */