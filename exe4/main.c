#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

const int BTN_PIN_R = 28;
const int BTN_PIN_G = 26;

const int LED_PIN_R = 4;
const int LED_PIN_G = 6;

QueueHandle_t xQueueButId;
QueueHandle_t xQueueBtn2;

QueueHandle_t xQueueEdgeR;
QueueHandle_t xQueueEdgeG;

void gpio_btn_isr(uint gpio, uint32_t events) {
    BaseType_t hpw = pdFALSE;
    uint8_t ev;

    if (gpio == BTN_PIN_R) {
        if (events & GPIO_IRQ_EDGE_FALL) { ev = 1; xQueueSendFromISR(xQueueEdgeR, &ev, &hpw); }
        if (events & GPIO_IRQ_EDGE_RISE) { ev = 2; xQueueSendFromISR(xQueueEdgeR, &ev, &hpw); }
    } else if (gpio == BTN_PIN_G) {
        if (events & GPIO_IRQ_EDGE_FALL) { ev = 1; xQueueSendFromISR(xQueueEdgeG, &ev, &hpw); }
        if (events & GPIO_IRQ_EDGE_RISE) { ev = 2; xQueueSendFromISR(xQueueEdgeG, &ev, &hpw); }
    }

    portYIELD_FROM_ISR(hpw);
}

void led_1_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_put(LED_PIN_R, 0);

    int delay = 0;
    for (;;) {
        if (xQueueReceive(xQueueButId, &delay, 0)) {
            printf("%d\n", delay);
        }

        if (delay > 0) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

void led_2_task(void *p) {
    gpio_init(LED_PIN_G);
    gpio_set_dir(LED_PIN_G, GPIO_OUT);
    gpio_put(LED_PIN_G, 0);

    int delay = 0;
    for (;;) {
        if (xQueueReceive(xQueueBtn2, &delay, 0)) {
            printf("%d\n", delay);
        }

        if (delay > 0) {
            gpio_put(LED_PIN_G, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_G, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

void btn_1_task(void *p) {
    int delay = 0;
    bool pressed = false;
    uint8_t ev;

    for (;;) {
        if (xQueueReceive(xQueueEdgeR, &ev, portMAX_DELAY)) {
            if (ev == 1) pressed = true;
            else if (ev == 2 && pressed) {
                pressed = false;
                delay = (delay < 1000) ? delay + 100 : 100;
                printf("delay btn R %d \n", delay);
                xQueueSend(xQueueButId, &delay, 0);
            }
        }
    }
}

void btn_2_task(void *p) {
    int delay = 0;
    bool pressed = false;
    uint8_t ev;

    for (;;) {
        if (xQueueReceive(xQueueEdgeG, &ev, portMAX_DELAY)) {
            if (ev == 1) pressed = true;
            else if (ev == 2 && pressed) {
                pressed = false;
                delay = (delay < 1000) ? delay + 100 : 100;
                printf("delay btn G %d \n", delay);
                xQueueSend(xQueueBtn2, &delay, 0);
            }
        }
    }
}

int main() {
    stdio_init_all();
    printf("Start RTOS \n");

    gpio_init(LED_PIN_R); gpio_set_dir(LED_PIN_R, GPIO_OUT); gpio_put(LED_PIN_R, 0);
    gpio_init(LED_PIN_G); gpio_set_dir(LED_PIN_G, GPIO_OUT); gpio_put(LED_PIN_G, 0);

    gpio_init(BTN_PIN_R); gpio_set_dir(BTN_PIN_R, GPIO_IN); gpio_pull_up(BTN_PIN_R);
    gpio_init(BTN_PIN_G); gpio_set_dir(BTN_PIN_G, GPIO_IN); gpio_pull_up(BTN_PIN_G);

    xQueueButId  = xQueueCreate(32, sizeof(int));
    xQueueBtn2   = xQueueCreate(32, sizeof(int));
    xQueueEdgeR  = xQueueCreate(8,  sizeof(uint8_t));
    xQueueEdgeG  = xQueueCreate(8,  sizeof(uint8_t));

    gpio_set_irq_enabled_with_callback(BTN_PIN_R,
        GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, gpio_btn_isr);
    gpio_set_irq_enabled(BTN_PIN_G,
        GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    xTaskCreate(led_1_task, "LED_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(btn_1_task, "BTN_Task 1", 256, NULL, 2, NULL);
    xTaskCreate(led_2_task, "LED_Task 2", 256, NULL, 1, NULL);
    xTaskCreate(btn_2_task, "BTN_Task 2", 256, NULL, 2, NULL);

    vTaskStartScheduler();
    while (true) ;
}