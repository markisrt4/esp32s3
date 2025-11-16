#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "ENC_TEST";

// Rotary encoder pins
#define ENC_CLK 5     // GPIO for CLK
#define ENC_DT  6     // GPIO for DT

// LED pin for testing (blinks on rotation)
#define LED_PIN 2

// Rotary state
static int last_clk = 1;

// ============================================================================
// FreeRTOS Tasks
// ============================================================================

// LED Flash Task -------------------------------------------------------------
void led_flash_task(void *arg)
{
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


// Rotary Encoder Task --------------------------------------------------------
void encoder_task(void *arg)
{
    while (1) {
        int clk = gpio_get_level(ENC_CLK);
        int dt  = gpio_get_level(ENC_DT);

        if (clk != last_clk) {
            if (clk == 1) {  // Only check on rising edge
                if (dt == 0) {
                    ESP_LOGI(TAG, "Clockwise");
                } else {
                    ESP_LOGI(TAG, "Counter-Clockwise");
                }

                // Flash LED on activity
                gpio_set_level(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(30));
                gpio_set_level(LED_PIN, 0);
            }
        }

        last_clk = clk;

        vTaskDelay(pdMS_TO_TICKS(2));  // VERY IMPORTANT: yields CPU
    }
}


// ============================================================================
// app_main
// ============================================================================
void app_main(void)
{
    ESP_LOGI(TAG, "Rotary encoder test starting...");

    // Configure encoder pins
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    io_conf.pin_bit_mask = (1ULL << ENC_CLK);
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << ENC_DT);
    gpio_config(&io_conf);

    // Configure LED
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    // Read initial state
    last_clk = gpio_get_level(ENC_CLK);

    // Create tasks
    xTaskCreatePinnedToCore(encoder_task, "encoder_task",
                            4096, NULL, 5, NULL, 1);

    xTaskCreatePinnedToCore(led_flash_task, "led_flash_task",
                            2048, NULL, 1, NULL, 1);

    // DELETE main task so watchdog stops tracking it
    vTaskDelete(NULL);
}
