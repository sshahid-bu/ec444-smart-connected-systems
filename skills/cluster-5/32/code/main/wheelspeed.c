//Myles Cork 10/27/20
//pcnt implementation based on https://github.com/espressif/esp-idf/blob/178b122c145c19e94ac896197a3a4a9d379cd618/examples/peripherals/pcnt/pulse_count_event/main/pcnt_event_example_main.c

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/pcnt.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define TIMER_DIVIDER         16    //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // to seconds
#define TIMER_INTERVAL_MSEC   (100)    // Sample test interval for the first timer
#define TEST_WITH_RELOAD      1     // Testing will be done with auto reload

// LED Output pins definitions
#define PDPIN  27

// Pulse Counter Functions /////////////////////////////////////////////////////
#define PCNT_H_LIM_VAL      1
#define PCNT_L_LIM_VAL     -1
#define PCNT_THRESH1_VAL    1
#define PCNT_THRESH0_VAL   -1
#define PCNT_INPUT_SIG_IO   27  // Pulse Input GPIO
#define PCNT_INPUT_CTRL_IO  5  // Control GPIO HIGH=count up, LOW=count down

#define WHEEL_DIAMETER_M .075;
#define ENCODER_SECTIONS 6;

xQueueHandle pcnt_evt_queue;   // A queue to handle pulse counter events

int64_t prevtime = 0;
float speed = 0;

/* A sample structure to pass events from the PCNT
 * interrupt handler to the main program.
 */
typedef struct {
    int unit;  // the PCNT unit that originated an interrupt
    uint32_t status; // information on the event type that caused the interrupt
} pcnt_evt_t;

// Pulse Counter ISR ///////////////////////////////////////////////////////////
static void IRAM_ATTR pcnt_example_intr_handler(void *arg)
{
    int pcnt_unit = (int)arg;
    pcnt_evt_t evt;
    evt.unit = pcnt_unit;
    /* Save the PCNT event type that caused an interrupt
       to pass it to the main program */
    pcnt_get_event_status(pcnt_unit, &evt.status);
    xQueueSendFromISR(pcnt_evt_queue, &evt, NULL);
}

// Pulse Counter INIT //////////////////////////////////////////////////////////
/* Initialize PCNT functions:
 *  - configure and initialize PCNT
 *  - set up the input filter
 *  - set up the counter events to watch
 */
static void pcnt_example_init(int unit)
{
    /* Prepare configuration for the PCNT unit */
    pcnt_config_t pcnt_config = {
      // Set PCNT input signal and control GPIOs
      .pulse_gpio_num = PCNT_INPUT_SIG_IO,
      .ctrl_gpio_num = PCNT_INPUT_CTRL_IO,
      .channel = PCNT_CHANNEL_0,
      .unit = unit,
      // What to do on the positive / negative edge of pulse input?
      .pos_mode = PCNT_COUNT_INC,   // Count up on the positive edge
      .neg_mode = PCNT_COUNT_DIS,   // Keep the counter value on the negative edge
      // What to do when control input is low or high?
      .lctrl_mode = PCNT_MODE_REVERSE, // Reverse counting direction if low
      .hctrl_mode = PCNT_MODE_KEEP,    // Keep the primary counter mode if high
      // Set the maximum and minimum limit values to watch
      .counter_h_lim = PCNT_H_LIM_VAL,
      .counter_l_lim = PCNT_L_LIM_VAL,
    };
    /* Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);

    /* Configure and enable the input filter */
    pcnt_set_filter_value(unit, 100);
    pcnt_filter_enable(unit);

    /* Set threshold 0 and 1 values and enable events to watch */
    pcnt_set_event_value(unit, PCNT_EVT_THRES_1, PCNT_THRESH1_VAL);
    pcnt_event_enable(unit, PCNT_EVT_THRES_1);
    pcnt_set_event_value(unit, PCNT_EVT_THRES_0, PCNT_THRESH0_VAL);
    pcnt_event_enable(unit, PCNT_EVT_THRES_0);
    /* Enable events on zero, maximum and minimum limit values */
    pcnt_event_enable(unit, PCNT_EVT_ZERO);
    pcnt_event_enable(unit, PCNT_EVT_H_LIM);
    pcnt_event_enable(unit, PCNT_EVT_L_LIM);

    /* Initialize PCNT's counter */
    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);

    /* Install interrupt service and add isr callback handler */
    pcnt_isr_service_install(0);
    pcnt_isr_handler_add(unit, pcnt_example_intr_handler, (void *)unit);

    /* Everything is set up, now go to counting */
    pcnt_counter_resume(unit);
}

void app_main() {
  int pcnt_unit = PCNT_UNIT_0;


  /* Initialize PCNT event queue and PCNT functions */
  pcnt_evt_queue = xQueueCreate(10, sizeof(pcnt_evt_t));
  pcnt_example_init(pcnt_unit);

  float wheel_circumference_m = M_PI * WHEEL_DIAMETER_M;

  int16_t count = 0;
  pcnt_evt_t evt;
  portBASE_TYPE res;
  prevtime = esp_timer_get_time();
  while (1) {
    /* Wait for the event information passed from PCNT's interrupt handler.
     * Once received, decode the event type and print it on the serial monitor.
     */
    if(xQueueReceive(pcnt_evt_queue, &evt, 1000 / portTICK_PERIOD_MS)){
      int64_t currtime = esp_timer_get_time();
      int64_t dt = currtime - prevtime;
      printf("dt: %lld\n", dt);
      prevtime = currtime;
      speed =  1/(float)dt * 1000000 * 1/6 * wheel_circumference_m;
      printf("Speed: %f\n", speed);
    }
  }
}
