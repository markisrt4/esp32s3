#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "ENC_TEST";

// GPIO pins
#define ENC_A_PIN   4
#define ENC_B_PIN   5
#define ENC_SW_PIN  6

#define LED_CW_PIN   2
#define LED_CCW_PIN  3

// State
static int last_state_A = 0;

static void flash_led(gpio_num_t pin, int times, int delay_ms)
{
    for (int i = 0; i < times; i++) {
        gpio_set_level(pin, 1);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        gpio_set_level(pin, 0);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Rotary encoder + LED test starting...");

    // Configure encoder pins
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << ENC_A_PIN) | (1ULL << ENC_B_PIN) | (1ULL << ENC_SW_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf);

    // Configure LEDs
    gpio_set_direction(LED_CW_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_CCW_PIN, GPIO_MODE_OUTPUT);

    // Read initial state
    last_state_A = gpio_get_level(ENC_A_PIN);

    while (1) {
        int state_A = gpio_get_level(ENC_A_PIN);
        int state_B = gpio_get_level(ENC_B_PIN);
        int sw = gpio_get_level(ENC_SW_PIN);

        // Detect rising edge on A
        if (state_A != last_state_A) {
            if (state_A == 1) {
                if (state_B == 0) {
                    ESP_LOGI(TAG, "Clockwise turn");
                    flash_led(LED_CW_PIN, 2, 50);
                } else {
                    ESP_LOGI(TAG, "Counter-Clockwise turn");
                    flash_led(LED_CCW_PIN, 2, 50);
                }
            }
        }

        last_state_A = state_A;

        // Push-button test
        if (sw == 0) {
            ESP_LOGI(TAG, "Encoder button pressed!");
            flash_led(LED_CW_PIN, 1, 80);
            flash_led(LED_CCW_PIN, 1, 80);
            vTaskDelay(pdMS_TO_TICKS(250));
        }

        vTaskDelay(pdMS_TO_TICKS(1)); // small debounce / sampling delay
    }
}

