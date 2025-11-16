#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "ENC_TEST";

// GPIOs
#define ENC_A_PIN 4
#define ENC_B_PIN 5
#define ENC_SW_PIN 6

#define LED_CW_PIN 2
#define LED_CCW_PIN 3

// LED action queue
typedef enum {
    LED_FLASH_CW,
    LED_FLASH_CCW,
    LED_FLASH_BOTH
} led_action_t;

static QueueHandle_t led_queue;

static int last_state_A = 0;

// LED Flash Task (runs independently, no blocking main task)
void led_task(void *arg)
{
    led_action_t action;
    while (1)
    {
        if (xQueueReceive(led_queue, &action, portMAX_DELAY)) 
        {
            switch (action) {
            case LED_FLASH_CW:
                for (int i = 0; i < 2; i++) {
                    gpio_set_level(LED_CW_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(40));
                    gpio_set_level(LED_CW_PIN, 0);
                    vTaskDelay(pdMS_TO_TICKS(40));
                }
                break;

            case LED_FLASH_CCW:
                for (int i = 0; i < 2; i++) {
                    gpio_set_level(LED_CCW_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(40));
                    gpio_set_level(LED_CCW_PIN, 0);
                    vTaskDelay(pdMS_TO_TICKS(40));
                }
                break;

            case LED_FLASH_BOTH:
                gpio_set_level(LED_CW_PIN, 1);
                gpio_set_level(LED_CCW_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_set_level(LED_CW_PIN, 0);
                gpio_set_level(LED_CCW_PIN, 0);
                break;
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting encoder test with watchdog-safe LED task");

    // Setup GPIOs
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << ENC_A_PIN) | (1ULL << ENC_B_PIN) | (1ULL << ENC_SW_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    gpio_set_direction(LED_CW_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_CCW_PIN, GPIO_MODE_OUTPUT);

    // LED queue
    led_queue = xQueueCreate(10, sizeof(led_action_t));

    // Start LED task
    xTaskCreatePinnedToCore(led_task, "led_task", 2048, NULL, 5, NULL, 1);

    last_state_A = gpio_get_level(ENC_A_PIN);

    while (1)
    {
        int state_A = gpio_get_level(ENC_A_PIN);
        int state_B = gpio_get_level(ENC_B_PIN);
        int sw = gpio_get_level(ENC_SW_PIN);

        if (state_A != last_state_A && state_A == 1)
        {
            if (state_B == 0) {
                ESP_LOGI(TAG, "CW");
                led_action_t a = LED_FLASH_CW;
                xQueueSend(led_queue, &a, 0);
            } else {
                ESP_LOGI(TAG, "CCW");
                led_action_t a = LED_FLASH_CCW;
                xQueueSend(led_queue, &a, 0);
            }
        }

        last_state_A = state_A;

        if (sw == 0) {
            ESP_LOGI(TAG, "Button!");
            led_action_t a = LED_FLASH_BOTH;
            xQueueSend(led_queue, &a, 0);
            vTaskDelay(pdMS_TO_TICKS(150));
        }

        vTaskDelay(pdMS_TO_TICKS(2));  // safe polling rate
    }
}
