#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

const int BTN_R_PIN = 28;
const int BTN_G_PIN = 26;

const int LED_R_PIN = 4;
const int LED_G_PIN = 6;

SemaphoreHandle_t xSemaphore_r;
SemaphoreHandle_t xSemaphore_g;


void gpio_btn_isr(uint gpio, uint32_t events) {
  if (gpio == BTN_R_PIN && (events & (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))) {
      xSemaphoreGiveFromISR(xSemaphore_r, 0;
  } else if (gpio == BTN_G_PIN && (events & (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))) {
      xSemaphoreGiveFromISR(xSemaphore_g, 0);
  }

  portYIELD_FROM_ISR(0);
}

void led_1_task(void *p) {
  for (;;) {
    xSemaphoreTake(xSemaphore_r, pdMS_TO_TICKS(250));
    gpio_put(LED_R_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_put(LED_R_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void led_2_task(void *p) {
  for (;;) {
    xSemaphoreTake(xSemaphore_g, pdMS_TO_TICKS(250));
    gpio_put(LED_G_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_put(LED_G_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void btn_1_task(void *p) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void btn_2_task(void *p) {
  for (;;) {  
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

int main(void) {
  stdio_init_all();

  gpio_init(LED_R_PIN); gpio_set_dir(LED_R_PIN, GPIO_OUT); gpio_put(LED_R_PIN, 0);
  gpio_init(LED_G_PIN); gpio_set_dir(LED_G_PIN, GPIO_OUT); gpio_put(LED_G_PIN, 0);

  gpio_init(BTN_R_PIN); gpio_set_dir(BTN_R_PIN, GPIO_IN); gpio_pull_up(BTN_R_PIN);
  gpio_init(BTN_G_PIN); gpio_set_dir(BTN_G_PIN, GPIO_IN); gpio_pull_up(BTN_G_PIN);

  xSemaphore_r = xSemaphoreCreateBinary();
  xSemaphore_g = xSemaphoreCreateBinary();

  gpio_set_irq_enabled_with_callback(BTN_R_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, gpio_btn_isr);
  gpio_set_irq_enabled(BTN_G_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

  xTaskCreate(btn_1_task, "BTN1", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
  xTaskCreate(led_1_task, "LED1", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
  xTaskCreate(btn_2_task, "BTN2", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
  xTaskCreate(led_2_task, "LED2", 512, NULL, tskIDLE_PRIORITY + 1, NULL);

  vTaskStartScheduler();
  for (;;);
}