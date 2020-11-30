// Skill Name: Lidar (using PWM)
// Name: Shazor Shahid
// Date: 2020-11-27

/* MCPWM basic config example
 * This example will show you how to use each submodule of MCPWM unit.
 * The example can't be used without modifying the code first.
 * Edit the macros at the top of mcpwm_example_basic_config.c to enable/disable the submodules which are used in the example.
 */

#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "esp_timer.h"
#include "soc/rtc.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#define CAP_SIG_NUM   3       //Three capture signals
#define CAP0_INT_EN   BIT(27) //Capture 0 interrupt bit
#define CAP1_INT_EN   BIT(27) //Capture 1 interrupt bit
#define GPIO_CAP0_IN  27      //Set GPIO 21 as CAP0
#define GPIO_CAP1_IN  27      //Set GPIO 27 as CAP1

typedef struct {
  uint32_t capture_signal;
  mcpwm_capture_signal_t sel_cap_signal;
  int64_t current_time;
} capture;

uint32_t *current_cap_value = NULL;
uint32_t *previous_cap_value = NULL;

xQueueHandle cap_queue1, cap_queue2;

static mcpwm_dev_t *MCPWM[2] = {&MCPWM0, &MCPWM1};

static void mcpwm_example_gpio_initialize(void) {
  printf("Initializing mcpwm gpio...\n");
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, GPIO_CAP0_IN);
  gpio_pulldown_en(GPIO_CAP0_IN);  //Enable pull down on CAP0 signal
}

// Set gpio 12 as our test signal that generates high-low waveform continuously,
// we generate a gpio_test_signal of 20ms on GPIO 12 and connect it to one of the capture signal,
// the disp_captured_function displays the time between rising edge
static void gpio_test_signal(void *arg) {
  printf("intializing test signal...\n");
  gpio_config_t gp;
  gp.intr_type = GPIO_INTR_DISABLE;
  gp.mode = GPIO_MODE_OUTPUT;
  gp.pin_bit_mask = GPIO_SEL_12;
  gpio_config(&gp);
  while (1) {
    //here the period of test signal is 20ms
    gpio_set_level(GPIO_NUM_12, 1); //Set high
    vTaskDelay(10 / portTICK_PERIOD_MS);         //delay of 10ms
    gpio_set_level(GPIO_NUM_12, 0); //Set low
    vTaskDelay(1000 / portTICK_PERIOD_MS);       //delay of 10ms
  }
}

// When interrupt occurs, receive the counter value and display the time between twoz edge.
static void disp_captured_signal(void *arg) {
  capture evt1;
  capture evt2;
  int64_t posEdge = 0; 
  int64_t negEdge = 0;
  while (1) {
    xQueueReceive(cap_queue1, &evt1, portMAX_DELAY);
    if (evt1.sel_cap_signal == MCPWM_SELECT_CAP0) { 
      posEdge = evt1.current_time;
    }
    xQueueReceive(cap_queue2, &evt2, portMAX_DELAY);
    if (evt2.sel_cap_signal == MCPWM_SELECT_CAP1) {
      negEdge = evt2.current_time;
    }
    int64_t signal_length = posEdge - negEdge;
    printf("CAP0 - CAP1: %lld us\n", signal_length);
  }
}

/* check for interrupt that triggers rising edge on CAP0 signal and according take action */
static void IRAM_ATTR isr_handler(void) {
  uint32_t mcpwm_intr_status;
  capture evt;
  mcpwm_intr_status = MCPWM[MCPWM_UNIT_0]->int_st.val; //Read interrupt status

  //calculate the interval in the ISR,
  //so that the interval will be always correct even when cap_queue is not handled in time and overflow.
  if (mcpwm_intr_status & CAP0_INT_EN) { //Check for interrupt on rising edge on CAP0 signal
    evt.current_time = esp_timer_get_time();
    current_cap_value[0] = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0); //get capture signal counter value
    evt.capture_signal = (current_cap_value[0] - previous_cap_value[0]) / (rtc_clk_apb_freq_get() / 1000000);
    previous_cap_value[0] = current_cap_value[0];
    evt.sel_cap_signal = MCPWM_SELECT_CAP0;
    xQueueSendFromISR(cap_queue1, &evt, NULL);
  }
  if (mcpwm_intr_status & CAP1_INT_EN) { //Check for interrupt on rising edge on CAP0 signal
    evt.current_time = esp_timer_get_time();
    current_cap_value[1] = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP1); //get capture signal counter value
    evt.capture_signal = (current_cap_value[1] - previous_cap_value[1]) / (rtc_clk_apb_freq_get() / 1000000);
    previous_cap_value[1] = current_cap_value[1];
    evt.sel_cap_signal = MCPWM_SELECT_CAP1;
    xQueueSendFromISR(cap_queue2, &evt, NULL);
  }
  MCPWM[MCPWM_UNIT_0]->int_clr.val = mcpwm_intr_status;
}

/* @brief Configure whole MCPWM module */
static void mcpwm_config(void *arg) {
  mcpwm_example_gpio_initialize();

  //mcpwm configuration
  printf("Configuring Initial Parameters of mcpwm...\n");
  mcpwm_config_t pwm_config;
  pwm_config.frequency = 1000;  //frequency = 1000Hz
  pwm_config.cmpr_a = 50.0;     //duty cycle of PWMxA = 60.0%
  pwm_config.cmpr_b = 50.0;     //duty cycle of PWMxb = 50.0%
  pwm_config.counter_mode = MCPWM_UP_COUNTER;
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config); //Configure PWM0A & PWM0B with above settings

  //Capture configuration
  //general practice, connect Capture to ext signal, measure time between rising or falling and take action 
  //capture signal on rising edge, prescale = 0 i.e. 800,000,000 counts is equal to one second
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_POS_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP1, MCPWM_NEG_EDGE, 0);
  //enable interrupt, so each this a rising edge occurs interrupt is triggered
  MCPWM[MCPWM_UNIT_0]->int_ena.val = CAP0_INT_EN | CAP1_INT_EN;
  mcpwm_isr_register(MCPWM_UNIT_0, isr_handler, NULL, ESP_INTR_FLAG_IRAM, NULL);  //Set ISR Handler
  vTaskDelete(NULL);
}

void app_main(void) {
  printf("Testing MCPWM...\n");
  cap_queue1 = xQueueCreate(1, sizeof(capture));
  cap_queue2 = xQueueCreate(1, sizeof(capture));
  
  current_cap_value = (uint32_t *)malloc(CAP_SIG_NUM*sizeof(uint32_t));
  previous_cap_value = (uint32_t *)malloc(CAP_SIG_NUM*sizeof(uint32_t));
  xTaskCreate(disp_captured_signal, "mcpwm_config", 4096, NULL, 5, NULL);
  xTaskCreate(gpio_test_signal, "gpio_test_signal", 4096, NULL, 5, NULL);
  xTaskCreate(mcpwm_config, "mcpwm_config", 4096, NULL, 5, NULL);
}
