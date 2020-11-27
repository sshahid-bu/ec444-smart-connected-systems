#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"


// servo definitions
#define STEER_MIN_PULSEWIDTH 1050
#define STEER_MID_PULSEWIDTH 1350
#define STEER_MAX_PULSEWIDTH 1650
#define STEER_MAX_DEGREE 30
#define STEERING_SERVO_PIN 25

#define ESC_MIN_PULSEWIDTH 1150
#define ESC_MID_PULSEWIDTH 1200
#define ESC_MAX_PULSEWIDTH 1250
#define ESC_SERVO_PIN 26

// mcpwm_pin_config_t pwmA_config = {
//
// };

static void mcpwm_steer_gpio_init() {
  printf("Initializing mcpwm servo control gpio.\n");
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, STEERING_SERVO_PIN);
}

static void mcpwm_esc_gpio_init() {
  printf("Initializing mcpwm servo control gpio.\n");
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, ESC_SERVO_PIN);
}

static uint32_t servo_per_degree_init(uint32_t degree_of_rotation)
{
    uint32_t cal_pulsewidth = 0;
    cal_pulsewidth = (STEER_MIN_PULSEWIDTH + (((STEER_MAX_PULSEWIDTH - STEER_MIN_PULSEWIDTH) * (degree_of_rotation)) / (STEER_MAX_DEGREE)));
    return cal_pulsewidth;
}

static void calibrateESC() {
  vTaskDelay(3000 / portTICK_PERIOD_MS);  // Give yourself time to turn on crawler (3s)
  mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, ESC_MID_PULSEWIDTH); // NEUTRAL signal in microseconds
  vTaskDelay(3100 / portTICK_PERIOD_MS); // Do for at least 3s, and leave in neutral state
}

void mcpwm_steer_control(void *arg)
{
    uint32_t angle, count;
    //1. mcpwm gpio initialization
    mcpwm_steer_gpio_init();

    //2. initial mcpwm configuration
    printf("Configuring Initial Parameters of mcpwm......\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50;    //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
    while (1) {
        for (count = 0; count < STEER_MAX_DEGREE; count++) {
            printf("Angle of rotation: %d\n", count);
            angle = servo_per_degree_init(count);
            printf("pulse width: %dus\n", angle);
            mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, angle);
            vTaskDelay(10);     //Add delay, since it takes time for servo to rotate, generally 100ms/60degree rotation at 5V
        }
    }
}

void mcpwm_esc_control(void *arg)
{
    uint32_t angle, count;
    //1. mcpwm gpio initialization
    mcpwm_esc_gpio_init();
    calibrateESC();

    //2. initial mcpwm configuration
    printf("Configuring Initial Parameters of mcpwm......\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50;    //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &pwm_config);    //Configure PWM0A & PWM0B with above settings

     while (1) {
       mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, ESC_MAX_PULSEWIDTH);
       vTaskDelay(10);     //Add delay, since it takes time for servo to rotate, generally 100ms/60degree rotation at 5V
    }
}

void app_main(void)
{
    printf("Testing servo motor.......\n");
    xTaskCreate(mcpwm_steer_control, "mcpwm_steer_control", 4096, NULL, 5, NULL);
    xTaskCreate(mcpwm_esc_control, "mcpwm_esc_control", 4096, NULL, 5, NULL);
}
