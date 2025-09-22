// exe5/main.c
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

typedef struct { uint8_t btn; uint8_t edge; } btn_evt_t;

QueueHandle_t xQueueBtn;
SemaphoreHandle_t xSemaphoreLedR;
SemaphoreHandle_t xSemaphoreLedY;

void gpio_btn_isr(uint gpio, uint32_t events) {
    BaseType_t hpw = pdFALSE;
    btn_evt_t ev;
    if (gpio == BTN_PIN_R) {
        ev.btn = 0;
        if (events & GPIO_IRQ_EDGE_FALL) { ev.edge = 1; xQueueSendFromISR(xQueueBtn, &ev, &hpw); }
        if (events & GPIO_IRQ_EDGE_RISE) { ev.edge = 2; xQueueSendFromISR(xQueueBtn, &ev, &hpw); }
    } else if (gpio == BTN_PIN_Y) {
        ev.btn = 1;
        if (events & GPIO_IRQ_EDGE_FALL) { ev.edge = 1; xQueueSendFromISR(xQueueBtn, &ev, &hpw); }
        if (events & GPIO_IRQ_EDGE_RISE) { ev.edge = 2; xQueueSendFromISR(xQueueBtn, &ev, &hpw); }
    }
    portYIELD_FROM_ISR(hpw);
}

void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_put(LED_PIN_R, 0);
    bool blinking = false;
    bool level = false;
    for (;;) {
        if (xSemaphoreTake(xSemaphoreLedR, 0) == pdTRUE) {
            blinking = !blinking;
            if (!blinking) { level = false; gpio_put(LED_PIN_R, 0); }
        }
        if (blinking) {
            level = !level;
            gpio_put(LED_PIN_R, level);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void led_y_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);
    gpio_put(LED_PIN_Y, 0);
    bool blinking = false;
    bool level = false;
    for (;;) {
        if (xSemaphoreTake(xSemaphoreLedY, 0) == pdTRUE) {
            blinking = !blinking;
            if (!blinking) { level = false; gpio_put(LED_PIN_Y, 0); }
        }
        if (blinking) {
            level = !level;
            gpio_put(LED_PIN_Y, level);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void btn_task(void* p) {
    bool pressed_r = false;
    bool pressed_y = false;
    btn_evt_t ev;
    for (;;) {
        if (xQueueReceive(xQueueBtn, &ev, portMAX_DELAY) == pdTRUE) {
            if (ev.btn == 0) {
                if (ev.edge == 1) pressed_r = true;
                else if (ev.edge == 2 && pressed_r) { pressed_r = false; xSemaphoreGive(xSemaphoreLedR); }
            } else {
                if (ev.edge == 1) pressed_y = true;
                else if (ev.edge == 2 && pressed_y) { pressed_y = false; xSemaphoreGive(xSemaphoreLedY); }
            }
        }
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN_R); gpio_set_dir(LED_PIN_R, GPIO_OUT); gpio_put(LED_PIN_R, 0);
    gpio_init(LED_PIN_Y); gpio_set_dir(LED_PIN_Y, GPIO_OUT); gpio_put(LED_PIN_Y, 0);

    gpio_init(BTN_PIN_R); gpio_set_dir(BTN_PIN_R, GPIO_IN); gpio_pull_up(BTN_PIN_R);
    gpio_init(BTN_PIN_Y); gpio_set_dir(BTN_PIN_Y, GPIO_IN); gpio_pull_up(BTN_PIN_Y);

    xQueueBtn      = xQueueCreate(16, sizeof(btn_evt_t));
    xSemaphoreLedR = xSemaphoreCreateBinary();
    xSemaphoreLedY = xSemaphoreCreateBinary();

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, gpio_btn_isr);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    xTaskCreate(btn_task,  "BTN_Task",  512, NULL, 2, NULL);
    xTaskCreate(led_r_task,"LED_R",     256, NULL, 1, NULL);
    xTaskCreate(led_y_task,"LED_Y",     256, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1) {}
}