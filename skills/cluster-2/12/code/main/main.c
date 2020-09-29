// Skill Name:      Battery Voltage Monitor
// Author, Email:   Shazor Shahid, sshahid@bu.edu
// Assignment:      EC444 Quest 2 Skill 12

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_adc_cal.h"

// 14-Segment Display
#define SLAVE_ADDR              0x70              // alphanumeric address
#define OSC                     0x21              // oscillator cmd
#define HT16K33_BLINK_DISPLAYON 0x01              // Display on cmd
#define HT16K33_BLINK_OFF       0                 // Blink off cmd
#define HT16K33_BLINK_CMD       0x80              // Blink cmd
#define HT16K33_CMD_BRIGHTNESS  0xE0              // Brightness cmd
// Master I2C
#define I2C_MASTER_SCL_IO       22                // gpio number for i2c clk
#define I2C_MASTER_SDA_IO       23                // gpio number for i2c data
#define I2C_MASTER_NUM          I2C_NUM_0         // i2c port
#define I2C_MASTER_TX_BUF_DIS   0                 // i2c master no buffer needed
#define I2C_MASTER_RX_BUF_DIS   0                 // i2c master no buffer needed
#define I2C_MASTER_FREQ_HZ      100000            // i2c master clock freq
#define WRITE_BIT               I2C_MASTER_WRITE  // i2c master write
#define READ_BIT                I2C_MASTER_READ   // i2c master read
#define ACK_CHECK_EN            true              // i2c master will check ack
#define ACK_CHECK_DIS           false             // i2c master will not check ack
#define ACK_VAL                 0x00              // i2c ack value
#define NACK_VAL                0xFF              // i2c nack value
// ADC
#define DEFAULT_VREF            1100              //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES           64                //Multisampling
// custom shortcuts
#define PAUSE                   1000 / portTICK_PERIOD_MS

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

static const uint16_t intfonttable[] = {
    0b0000110000111111, // 0
    0b0000000000000110, // 1
    0b0000000011011011, // 2
    0b0000000010001111, // 3
    0b0000000011100110, // 4
    0b0010000001101001, // 5
    0b0000000011111101, // 6
    0b0000000000000111, // 7
    0b0000000011111111, // 8
    0b0000000011101111, // 9
};

int testConnection(uint8_t devAddr, int32_t timeout) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  int err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return err;
}
int alpha_oscillator() {
  // Turn on oscillator for alpha display
  int ret;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, OSC, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  vTaskDelay(200 / portTICK_RATE_MS);
  return ret;
}
int no_blink() {
  // Set blink rate to off
  int ret;
  i2c_cmd_handle_t cmd2 = i2c_cmd_link_create();
  i2c_master_start(cmd2);
  i2c_master_write_byte(cmd2, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd2, HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (HT16K33_BLINK_OFF << 1), ACK_CHECK_EN);
  i2c_master_stop(cmd2);
  ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd2, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd2);
  vTaskDelay(200 / portTICK_RATE_MS);
  return ret;
}
int set_brightness_max(uint8_t val) {
  // Set Brightness
  int ret;
  i2c_cmd_handle_t cmd3 = i2c_cmd_link_create();
  i2c_master_start(cmd3);
  i2c_master_write_byte(cmd3, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd3, HT16K33_CMD_BRIGHTNESS | val, ACK_CHECK_EN);
  i2c_master_stop(cmd3);
  ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd3, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd3);
  vTaskDelay(200 / portTICK_RATE_MS);
  return ret;
}
static void i2c_master_init() {
  // Debug
  printf("\n>> i2c Config\n");
  int err;
  // Port configuration
  int i2c_master_port = I2C_MASTER_NUM;
  /// Define I2C configurations
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;                    // Master mode
  conf.sda_io_num = I2C_MASTER_SDA_IO;            // Default SDA pin
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;        // Internal pullup
  conf.scl_io_num = I2C_MASTER_SCL_IO;            // Default SCL pin
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;        // Internal pullup
  conf.master.clk_speed = I2C_MASTER_FREQ_HZ;     // CLK frequency
  err = i2c_param_config(i2c_master_port, &conf); // Configure
  if (err == ESP_OK) {printf("- parameters: ok\n");}

  // Install I2C driver
  err = i2c_driver_install(i2c_master_port, conf.mode,
    I2C_MASTER_RX_BUF_DIS, I2C_MASTER_TX_BUF_DIS, 0
  );
  if (err == ESP_OK) {printf("- initialized: yes\n\n");}

  // i2c_set_data_mode(i2c_master_port, I2C_DATA_MODE_LSB_FIRST, I2C_DATA_MODE_LSB_FIRST);

  // Dat in MSB mode
  i2c_set_data_mode(i2c_master_port, I2C_DATA_MODE_MSB_FIRST, I2C_DATA_MODE_MSB_FIRST);
}
static void i2c_scanner() {
  int32_t scanTimeout = 1000;
  printf("\n>> I2C scanning ..."  "\n");
  uint8_t count = 0;
  for (uint8_t i = 1; i < 127; i++) {
    // printf("0x%X%s",i,"\n");
    if (testConnection(i, scanTimeout) == ESP_OK) {
      printf( "- Device found at address: 0x%X%s", i, "\n");
      count++;
    }
  }
  if (count == 0) { printf("- No I2C devices found!" "\n"); }
  printf("\n");
}

static void i2c_display(void *arg) {
  // Debug
  int ret;
  printf(">> Test Alphanumeric Display: \n");
  // Set up routines
  ret = alpha_oscillator();       // Turn on alpha oscillator
  if(ret == ESP_OK) {printf("- oscillator: ok \n");}
  ret = no_blink();               // Set display blink off
  if(ret == ESP_OK) {printf("- blink: off \n");}
  ret = set_brightness_max(0xF);  // Set brightness max
  if(ret == ESP_OK) {printf("- brightness: max \n");}

  uint16_t displayBuffer[8];
  displayBuffer[3] = intfonttable[0];
  displayBuffer[2] = intfonttable[0];
  displayBuffer[1] = intfonttable[0];
  displayBuffer[0] = intfonttable[0];

  i2c_cmd_handle_t cmd4 = i2c_cmd_link_create();
  i2c_master_start(cmd4);
  i2c_master_write_byte(cmd4, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd4, (uint8_t)0x00, ACK_CHECK_EN);
  for (uint8_t i=0; i<8; i++) {
    i2c_master_write_byte(cmd4, displayBuffer[i] & 0xFF, ACK_CHECK_EN);
    i2c_master_write_byte(cmd4, displayBuffer[i] >> 8, ACK_CHECK_EN);
  }
  i2c_master_stop(cmd4);
  ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd4, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd4);

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
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
  print_char_val_type(val_type);

  //Continuously sample ADC1
  while (1) {
    uint32_t adc_reading = 0;
    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
      if (unit == ADC_UNIT_1) {
        adc_reading += adc1_get_raw((adc1_channel_t)channel);
      } else {
        int raw;
        adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
        adc_reading += raw;
      }
    }

    adc_reading /= NO_OF_SAMPLES;
    //Convert adc_reading to voltage in mV
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    printf("Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);

    int digit = voltage % 10;
    displayBuffer[3] = intfonttable[digit];
    voltage /= 10; digit = voltage % 10;
    displayBuffer[2] = intfonttable[digit];
    voltage /= 10; digit = voltage % 10;
    displayBuffer[1] = intfonttable[digit];
    voltage /= 10; digit = voltage % 10;
    displayBuffer[0] = intfonttable[digit];

    i2c_cmd_handle_t cmd4 = i2c_cmd_link_create();
    i2c_master_start(cmd4);
    i2c_master_write_byte(cmd4, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd4, (uint8_t)0x00, ACK_CHECK_EN);
    for (uint8_t i=0; i<8; i++) {
      i2c_master_write_byte(cmd4, displayBuffer[i] & 0xFF, ACK_CHECK_EN);
      i2c_master_write_byte(cmd4, displayBuffer[i] >> 8, ACK_CHECK_EN);
    }
    i2c_master_stop(cmd4);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd4, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd4);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void init(void) {
  // I2C CONFIG
  i2c_master_init();
  i2c_scanner();
}

void app_main(void) {
  init();
  xTaskCreate(i2c_display, "i2c_display", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
}
