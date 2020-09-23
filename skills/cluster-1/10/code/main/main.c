// Skill Name:      RTOS TASKs - Free RTOS
// Author, Email:   Shazor Shahid, sshahid@bu.edu
// Assignment:      EC444 Quest 1 Skill 10

#include <stdio.h>
#include "string.h"
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/uart.h"
#include "sdkconfig.h"

static const int RX_BUF_SIZE = 1024;

// 14-Segment Display
#define SLAVE_ADDR                         0x70 // alphanumeric address
#define OSC                                0x21 // oscillator cmd
#define HT16K33_BLINK_DISPLAYON            0x01 // Display on cmd
#define HT16K33_BLINK_OFF                  0    // Blink off cmd
#define HT16K33_BLINK_CMD                  0x80 // Blink cmd
#define HT16K33_CMD_BRIGHTNESS             0xE0 // Brightness cmd

// Master I2C
#define I2C_EXAMPLE_MASTER_SCL_IO          22   // gpio number for i2c clk
#define I2C_EXAMPLE_MASTER_SDA_IO          23   // gpio number for i2c data
#define I2C_EXAMPLE_MASTER_NUM             I2C_NUM_0  // i2c port
#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE  0    // i2c master no buffer needed
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE  0    // i2c master no buffer needed
#define I2C_EXAMPLE_MASTER_FREQ_HZ         100000     // i2c master clock freq
#define WRITE_BIT                          I2C_MASTER_WRITE // i2c master write
#define READ_BIT                           I2C_MASTER_READ  // i2c master read
#define ACK_CHECK_EN                       true // i2c master will check ack
#define ACK_CHECK_DIS                      false// i2c master will not check ack
#define ACK_VAL                            0x00 // i2c ack value
#define NACK_VAL                           0xFF // i2c nack value

#define TXD_PIN (GPIO_NUM_13)
#define RXD_PIN (GPIO_NUM_12)
#define N1 26     // A0
#define N2 25     // A1
#define N3 27     // 27
#define N4 21     // 21
#define BUTTON 32 // button attached to pin 13
#define RESET(x)    gpio_reset_pin(x);
#define ON(x)       gpio_set_level(x, 1)
#define OFF(x)      gpio_set_level(x, 0)
#define PAUSE       250 / portTICK_PERIOD_MS
#define BOOL_TOG(x) (x = !x)

// global variables to be used by tasks
bool countDirection = 1;

static void i2c_example_master_init(){
    // Debug
    printf("\n>> i2c Config\n");
    int err;

    // Port configuration
    int i2c_master_port = I2C_EXAMPLE_MASTER_NUM;

    /// Define I2C configurations
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;                              // Master mode
    conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;              // Default SDA pin
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;                  // Internal pullup
    conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;              // Default SCL pin
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;                  // Internal pullup
    conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ;       // CLK frequency
    err = i2c_param_config(i2c_master_port, &conf);           // Configure
    if (err == ESP_OK) {printf("- parameters: ok\n");}

    // Install I2C driver
    err = i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                       I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0);
    // i2c_set_data_mode(i2c_master_port,I2C_DATA_MODE_LSB_FIRST,I2C_DATA_MODE_LSB_FIRST);
    if (err == ESP_OK) {printf("- initialized: yes\n\n");}

    // Dat in MSB mode
    i2c_set_data_mode(i2c_master_port, I2C_DATA_MODE_MSB_FIRST, I2C_DATA_MODE_MSB_FIRST);
}

static const uint16_t alphafonttable[] = {
    0b0000000000000001, 0b0000000000000010, 0b0000000000000100,
    0b0000000000001000, 0b0000000000010000, 0b0000000000100000,
    0b0000000001000000, 0b0000000010000000, 0b0000000100000000,
    0b0000001000000000, 0b0000010000000000, 0b0000100000000000,
    0b0001000000000000, 0b0010000000000000, 0b0100000000000000,
    0b1000000000000000, 0b0000000000000000, 0b0000000000000000,
    0b0000000000000000, 0b0000000000000000, 0b0000000000000000,
    0b0000000000000000, 0b0000000000000000, 0b0000000000000000,
    0b0001001011001001, 0b0001010111000000, 0b0001001011111001,
    0b0000000011100011, 0b0000010100110000, 0b0001001011001000,
    0b0011101000000000, 0b0001011100000000,
    0b0000000000000000, //
    0b0000000000000110, // !
    0b0000001000100000, // "
    0b0001001011001110, // #
    0b0001001011101101, // $
    0b0000110000100100, // %
    0b0010001101011101, // &
    0b0000010000000000, // '
    0b0010010000000000, // (
    0b0000100100000000, // )
    0b0011111111000000, // *
    0b0001001011000000, // +
    0b0000100000000000, // ,
    0b0000000011000000, // -
    0b0000000000000000, // .
    0b0000110000000000, // /
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
    0b0001001000000000, // :
    0b0000101000000000, // ;
    0b0010010000000000, // <
    0b0000000011001000, // =
    0b0000100100000000, // >
    0b0001000010000011, // ?
    0b0000001010111011, // @
    0b0000000011110111, // A
    0b0001001010001111, // B
    0b0000000000111001, // C
    0b0001001000001111, // D
    0b0000000011111001, // E
    0b0000000001110001, // F
    0b0000000010111101, // G
    0b0000000011110110, // H
    0b0001001000000000, // I
    0b0000000000011110, // J
    0b0010010001110000, // K
    0b0000000000111000, // L
    0b0000010100110110, // M
    0b0010000100110110, // N
    0b0000000000111111, // O
    0b0000000011110011, // P
    0b0010000000111111, // Q
    0b0010000011110011, // R
    0b0000000011101101, // S
    0b0001001000000001, // T
    0b0000000000111110, // U
    0b0000110000110000, // V
    0b0010100000110110, // W
    0b0010110100000000, // X
    0b0001010100000000, // Y
    0b0000110000001001, // Z
    0b0000000000111001, // [
    0b0010000100000000, //
    0b0000000000001111, // ]
    0b0000110000000011, // ^
    0b0000000000001000, // _
    0b0000000100000000, // `
    0b0001000001011000, // a
    0b0010000001111000, // b
    0b0000000011011000, // c
    0b0000100010001110, // d
    0b0000100001011000, // e
    0b0000000001110001, // f
    0b0000010010001110, // g
    0b0001000001110000, // h
    0b0001000000000000, // i
    0b0000000000001110, // j
    0b0011011000000000, // k
    0b0000000000110000, // l
    0b0001000011010100, // m
    0b0001000001010000, // n
    0b0000000011011100, // o
    0b0000000101110000, // p
    0b0000010010000110, // q
    0b0000000001010000, // r
    0b0010000010001000, // s
    0b0000000001111000, // t
    0b0000000000011100, // u
    0b0010000000000100, // v
    0b0010100000010100, // w
    0b0010100011000000, // x
    0b0010000000001100, // y
    0b0000100001001000, // z
    0b0000100101001001, // {
    0b0001001000000000, // |
    0b0010010010001001, // }
    0b0000010100100000, // ~
    0b0011111111111111,
};

int testConnection(uint8_t devAddr, int32_t timeout) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    int err = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return err;
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

int alpha_oscillator() {
  // Turn on oscillator for alpha display
  int ret;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, OSC, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
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
  ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd2, 1000 / portTICK_RATE_MS);
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
  ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd3, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd3);
  vTaskDelay(200 / portTICK_RATE_MS);
  return ret;
}

void init(void) {
  ////// PIN CONFIG
  RESET(N1);
  RESET(N2);
  RESET(N3);
  RESET(N4);
  RESET(BUTTON);

  gpio_set_direction(N1, GPIO_MODE_OUTPUT);
  gpio_set_direction(N2, GPIO_MODE_OUTPUT);
  gpio_set_direction(N3, GPIO_MODE_OUTPUT);
  gpio_set_direction(N4, GPIO_MODE_OUTPUT);
  
  gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
  gpio_pulldown_en(BUTTON);

  ////// I2C CONFIG
  i2c_example_master_init();
  i2c_scanner();

  ////// UART CONFIG
  const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
  };

  uart_driver_install(UART_NUM_0, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
  uart_param_config(UART_NUM_0, &uart_config);
  uart_set_pin(UART_NUM_0, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

/* int sendData(const char* logName, const char* data) {
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_0, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}
*/

/* static void tx_task(void *arg) {
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG, "Hello world");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
*/

/* static void rx_task(void *arg) {
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}
*/

static void ledBinaryCount(void *arg) {

  int binaryCount = 0; vTaskDelay(PAUSE);

  while(1) {
    // loop to 0 after 15
    if (countDirection) {
      if (binaryCount >= 15)  { binaryCount = -1; vTaskDelay(PAUSE/2); }
      binaryCount++;
    }
    // loop to 15 after 0
    else if (!countDirection) { 
      if (binaryCount <= 0)   { binaryCount = 16; vTaskDelay(PAUSE/2); } 
      binaryCount --;
    }

    while(binaryCount < 16) {
      OFF(N1); OFF(N2); OFF(N3); OFF(N4);
      switch(binaryCount) {
        case 0:
          printf("binaryCount: 0000\n"); vTaskDelay(PAUSE);
          break;
        case 1:
          ON(N1);
          printf("binaryCount: 0001\n"); vTaskDelay(PAUSE);
          break;
        case 2:
          ON(N2);
          printf("binaryCount: 0010\n"); vTaskDelay(PAUSE);
          break;
        case 3:
          ON(N1); ON(N2);
          printf("binaryCount: 0011\n"); vTaskDelay(PAUSE);
          break;
        case 4:
          ON(N3);
          printf("binaryCount: 0100\n"); vTaskDelay(PAUSE);
          break;
        case 5:
          ON(N3); ON(N1);
          printf("binaryCount: 0101\n"); vTaskDelay(PAUSE);
          break;
        case 6:
          ON(N3); ON(N2);
          printf("binaryCount: 0110\n"); vTaskDelay(PAUSE);
          break;
        case 7:
          ON(N3); ON(N2); ON(N1);
          printf("binaryCount: 0111\n"); vTaskDelay(PAUSE);
          break;
        case 8:
          ON(N4);
          printf("binaryCount: 1000\n"); vTaskDelay(PAUSE);
          break;
        case 9:
          ON(N4); ON(N1);
          printf("binaryCount: 1001\n"); vTaskDelay(PAUSE);
          break;
        case 10:
          ON(N4); ON(N2);
          printf("binaryCount: 1010\n"); vTaskDelay(PAUSE);
          break;
        case 11:
          ON(N4); ON(N2); ON(N1);
          printf("binaryCount: 1011\n"); vTaskDelay(PAUSE);
          break;
        case 12:
          ON(N4); ON(N3);
          printf("binaryCount: 1100\n"); vTaskDelay(PAUSE);
          break;
        case 13:
          ON(N4); ON(N3); ON(N1);
          printf("binaryCount: 1101\n"); vTaskDelay(PAUSE);
          break;
        case 14:
          ON(N4); ON(N3); ON(N2);
          printf("binaryCount: 1110\n"); vTaskDelay(PAUSE);
          break;
        case 15:
          ON(N4); ON(N3); ON(N2); ON(N1);
          printf("binaryCount: 1111\n"); vTaskDelay(PAUSE);
          break;
        } break;
    }
  }
}

static void buttonPress(void *arg) {
  while (1) {
    if ( gpio_get_level(BUTTON) ) { countDirection = !countDirection; }
    vTaskDelay(10);
  } 
}

static void dispDirection(void *arg) {
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

  while (1) {
    uint16_t displayBuffer[8];
    if (countDirection) {
      displayBuffer[0] = alphafonttable[( (int) 'U') ];
      displayBuffer[1] = alphafonttable[( (int) 'P') ];
      displayBuffer[2] = alphafonttable[( (int) ' ') ];
      displayBuffer[3] = alphafonttable[( (int) ' ') ];
    } else if (!countDirection) {
      displayBuffer[0] = alphafonttable[( (int) 'D') ];
      displayBuffer[1] = alphafonttable[( (int) 'O') ];
      displayBuffer[2] = alphafonttable[( (int) 'W') ];
      displayBuffer[3] = alphafonttable[( (int) 'N') ];
    }
    // Send commands characters to display over I2C
    i2c_cmd_handle_t cmd4 = i2c_cmd_link_create();
    i2c_master_start(cmd4);
    i2c_master_write_byte(cmd4, ( SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd4, (uint8_t)0x00, ACK_CHECK_EN);
    for (uint8_t i=0; i<8; i++) {
      i2c_master_write_byte(cmd4, displayBuffer[i] & 0xFF, ACK_CHECK_EN);
      i2c_master_write_byte(cmd4, displayBuffer[i] >> 8, ACK_CHECK_EN);
    }
    i2c_master_stop(cmd4);
    ret = i2c_master_cmd_begin(I2C_EXAMPLE_MASTER_NUM, cmd4, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd4);
  }
}

void app_main(void) {
  init();
  xTaskCreate(dispDirection, "dispDirection", 1024*2, NULL, configMAX_PRIORITIES-2, NULL);
  xTaskCreate(ledBinaryCount, "ledBinaryCount", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
  xTaskCreate(buttonPress, "buttonPress", 1024*2, NULL, configMAX_PRIORITIES, NULL);
  // xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
  // xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
}