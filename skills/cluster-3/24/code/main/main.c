// Skill Name:      Use PWM to Control Power Delivery to LED
// Author, Email:   Shazor Shahid, sshahid@bu.edu
// Assignment:      EC444 Quest 3 Skill 24

// Adapted from ESP-IDF LEDC (LED Controller) fade example

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_err.h"

#define LEDC_TIMER       LEDC_TIMER_1
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_CH2_GPIO    (4) // HUZZAH32 A5
#define LEDC_CH2_CHANNEL LEDC_CHANNEL_2
#define LEDC_TIMER_BIT   LEDC_TIMER_10_BIT

// UART
#define UART_NUM UART_NUM_0
#define ECHO_TXD (UART_PIN_NO_CHANGE)
#define ECHO_RXD (UART_PIN_NO_CHANGE)
#define ECHO_RTS (UART_PIN_NO_CHANGE)
#define ECHO_CTS (UART_PIN_NO_CHANGE)
#define BUF_SIZE (128)



void app_main(void) {
  // Prepare and set configuration of timers that will be used by LED Controller 
  ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_TIMER_BIT, // resolution of PWM duty
    .freq_hz = 5000,                      // frequency of PWM signal
    .speed_mode = LEDC_MODE,           // timer mode
    .timer_num = LEDC_TIMER,           // timer index
    .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
  };
  ledc_channel_config_t ledc_channel = {
    .channel    = LEDC_CH2_CHANNEL,
    .duty       = 0,
    .gpio_num   = LEDC_CH2_GPIO,
    .speed_mode = LEDC_MODE,
    .hpoint     = 0,
    .timer_sel  = LEDC_TIMER
  };
  ledc_timer_config(&ledc_timer); // Set configuration of timer0 for high speed channels
  ledc_channel_config(&ledc_channel); // Set LED Controller with previously prepared configuration
  ledc_fade_func_install(0); // Initialize fade service.

  uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
  };
  uart_param_config(UART_NUM, &uart_config);
  uart_set_pin(UART_NUM, ECHO_TXD, ECHO_RXD, ECHO_RTS, ECHO_CTS);
  uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

  // ledc_set_duty, ledc_update_duty
  // lec_set_fade_with_time, ledc_fade_start
  char userInput[4];
  printf("\nEnter duty cycle time (in sec, max 8):\n");

  while (1) {
    int len = uart_read_bytes(UART_NUM, userInput, BUF_SIZE, 20/portTICK_RATE_MS);
    if (len > 0) {
      int sec = atoi(userInput);
      double pwm = sec * 100; 
      const int interval = 250;

      printf("LEDC to duty = %f\n", pwm);
      ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
      ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);

      for (int i = 1; i < sec+1; i++ ) {
        ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, i * pwm/sec);
        ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
        printf("going up %d\n", i);
        vTaskDelay(interval / portTICK_RATE_MS);
      }
      for (int i = sec-1; i > 0; i-- ) {
        ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, i * pwm/sec);
        ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
        printf("going down %d\n", i);
        vTaskDelay(interval / portTICK_RATE_MS);
      }
      ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
      ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
      printf("Enter duty cycle time: \n");

    }
  }
}