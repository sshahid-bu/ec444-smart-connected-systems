// Skill Name:      Ultrasonic Range Sensor
// Author, Email:   Shazor Shahid, sshahid@bu.edu
// Assignment:      EC444 Quest 2 Skill 14

/*

Reference:
https://www.maxbotix.com/documents/LV-MaxSonar-EZ_Datasheet.pdf

*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_adc_cal.h"

// ADC
#define DEFAULT_VREF  1100  //Use adc2_vref_to_gpio() to obtain a better estimate
#define SAMPLES       32    //Multisampling
// custom shortcuts
#define PAUSE         2000 / portTICK_PERIOD_MS // 2 seconds

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;//GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;

static void check_efuse(void) {
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}
static void print_char_val_type(esp_adc_cal_value_t val_type) {
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    printf("Characterized using Two Point Value\n");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    printf("Characterized using eFuse Vref\n");
  } else {
    printf("Characterized using Default Vref\n");
  }
}

static void ultrasonic_range(void *args) {
  //Check if Two Point or Vref are burned into eFuse
  check_efuse();
  //Configure ADC
  if (unit == ADC_UNIT_1) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(channel, atten);
  } else { adc2_config_channel_atten((adc2_channel_t)channel, atten); }
  //Characterize ADC
  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
    unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
  print_char_val_type(val_type);

  //Continuously sample ADC1
  while (1) {
    float adc_reading = 0;

    //Multisampling
    for (int i = 0; i < SAMPLES; i++) {
      if (unit == ADC_UNIT_1) {
        adc_reading += adc1_get_raw((adc1_channel_t)channel);
      } else {
        int raw;
        adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
        adc_reading += raw;
      }
    }
    adc_reading /= SAMPLES;
    uint32_t voltage;
    voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    printf("Raw: %f\tVoltage: %dmV\n", adc_reading, voltage);
    float distance;
    distance = voltage / 6.4;
    printf("distance: %f feet\n", distance/12);
 
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void app_main(void) {
  xTaskCreate(ultrasonic_range, "ultrasonic_range", 2048, NULL, configMAX_PRIORITIES-1, NULL);
}
