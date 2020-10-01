// Skill Name:      Thermistor
// Author, Email:   Shazor Shahid, sshahid@bu.edu
// Assignment:      EC444 Quest 2 Skill 13

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

// ADC
#define DEFAULT_VREF  1100  //Use adc2_vref_to_gpio() to obtain a better estimate
#define SAMPLES       32    //Multisampling
// custom shortcuts
#define PAUSE         2000 / portTICK_PERIOD_MS // 2 seconds
#define ADC_MAX       3134                      // self-measured max voltage using db11 and 3.3v 

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel  = ADC_CHANNEL_6; //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten      = ADC_ATTEN_DB_11;   // 150 ~ 2450
static const adc_unit_t unit        = ADC_UNIT_1;

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

static void read_temperature(void *args) {
  //Check if Two Point or Vref are burned into eFuse
  check_efuse();
  //Configure ADC
  if (unit == ADC_UNIT_1) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(channel, atten);
  } else {
    adc2_config_channel_atten((adc2_channel_t)channel, atten);
  }
  //Characterize ADC
  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_value_t val_type =
    esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
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

    //Convert adc_reading to voltage in mV
    uint32_t voltage;
    voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    printf("Raw: %f\tVoltage: %dmV\n", adc_reading, voltage);

    //const float B         = 3435;
    //const float t_room    = 298.15;
    //const float resistor  = 10000;
    //float resistance;

    //resistance = resistor * voltage / ADC_MAX - voltage;
    //printf("resistance = %f\n", resistance);

    //float temperature;
    //temperature = (B * t_room) / (B - (t_room * log(resistor/resistance)));
    //printf("temperature: %f\n", temperature-273.15);
  
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 3410 raw value for 0 Celsius

  }
}

void app_main(void) {
  xTaskCreate(read_temperature, "read_temperature", 2048, NULL, configMAX_PRIORITIES-1, NULL);
}
