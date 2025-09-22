#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_G = 26;

const int LED_PIN_R = 4;
const int LED_PIN_G = 6;

static SemaphoreHandle_t xSemaphore_r;
static SemaphoreHandle_t xSemaphore_g;

static void gpio_btn_isr(uint gpio, uint32_t events) {
    BaseType_t hpw = pdFALSE;
    if (gpio == BTN_PIN_R && (events & (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))) {
        xSemaphoreGiveFromISR(xSemaphore_r, &hpw);
    } else if (gpio == BTN_PIN_G && (events & (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))) {
        xSemaphoreGiveFromISR(xSemaphore_g, &hpw);
    }
    portYIELD_FROM_ISR(hpw);
}

void led_1_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);
    for (;;) {
        xSemaphoreTake(xSemaphore_r, portMAX_DELAY);
        gpio_put(LED_PIN_R, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_put(LED_PIN_R, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void led_2_task(void *p) {
    gpio_init(LED_PIN_G);
    gpio_set_dir(LED_PIN_G, GPIO_OUT);
    for (;;) {
        xSemaphoreTake(xSemaphore_g, portMAX_DELAY);
        gpio_put(LED_PIN_G, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_put(LED_PIN_G, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void btn_1_task(void *p) { for (;;) vTaskDelay(pdMS_TO_TICKS(1000)); }
void btn_2_task(void *p) { for (;;) vTaskDelay(pdMS_TO_TICKS(1000)); }

int main(void) {
    stdio_init_all();

    gpio_init(BTN_PIN_R); gpio_set_dir(BTN_PIN_R, GPIO_IN); gpio_pull_up(BTN_PIN_R);
    gpio_init(BTN_PIN_G); gpio_set_dir(BTN_PIN_G, GPIO_IN); gpio_pull_up(BTN_PIN_G);

    xSemaphore_r = xSemaphoreCreateBinary();
    xSemaphore_g = xSemaphoreCreateBinary();

    gpio_set_irq_enabled_with_callback(BTN_PIN_R,
        GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, gpio_btn_isr);
    gpio_set_irq_enabled(BTN_PIN_G, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    xTaskCreate(btn_1_task, "BTN1", 256, NULL, 2, NULL);
    xTaskCreate(led_1_task, "LED1", 256, NULL, 1, NULL);
    xTaskCreate(btn_2_task, "BTN2", 256, NULL, 2, NULL);
    xTaskCreate(led_2_task, "LED2", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    while (true) ;
}