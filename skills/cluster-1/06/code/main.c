// Console I/O (using UART)
// Author: Shazor Shahid
// Date: 9/12/2020
// Source code used: UART echo example

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "driver/uart.h"
#include "driver/gpio.h"


#define EX_UART_NUM UART_NUM_0
#define ECHO_TEST_TXD  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_RXD  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)

#define RESET(x)    gpio_reset_pin(x);
#define ON(x)       gpio_set_level(x, 1)
#define OFF(x)      gpio_set_level(x, 0)
#define PAUSE       1000 / portTICK_PERIOD_MS
#define PIN         (gpio_num_t) 21

#define BUF_SIZE (1024)

void app_main(void) {

    // Configure parameters of the UART driver communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_param_config(EX_UART_NUM, &uart_config);
    uart_set_pin(EX_UART_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    RESET(PIN);
    gpio_set_direction(PIN, GPIO_MODE_OUTPUT);

    // Configure a temporary buffer for the incoming data
    // uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    char        input[BUF_SIZE];
    char        parse[BUF_SIZE];
    int         len;
    int         i           = 0;
    int         mode        = 0;    // 0 = toggle mode, 1 = echo mode, 2 = int-hex mode
    bool        ledOnOff    = 0;    // 0 = off, 1 = on
    const int   decimals    = 2;
    

    while (1) {
            // Read
        // int len = uart_read_bytes(EX_UART_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS); 
            // Write
        // if (len > 0) { printf("Read: %c \n", *data); }
        // int len = uart_read_bytes(EX_UART_NUM, input, BUF_SIZE, 20 / portTICK_RATE_MS);
        // if (len > 0) { printf("Read %c\n", *input); }

        printf("toggle mode\n");
        while (mode == 0) {
            len = uart_read_bytes(EX_UART_NUM, input, BUF_SIZE, 20 / portTICK_RATE_MS);
            if (len > 0) {
                if (input[0] == 't') {
                    printf("Read: %c\n", input[0]);

                    ledOnOff = !ledOnOff;
                    if (ledOnOff) { ON(PIN); }
                    else if (!ledOnOff) { OFF(PIN); }
                }
                else if (input[0] == 's') { mode = 1; break; }
            }
            uart_flush(EX_UART_NUM);
        }

        printf("echo mode\n"); i = 0;
        while (mode == 1) {
            len = uart_read_bytes(EX_UART_NUM, input, BUF_SIZE, 20 / portTICK_RATE_MS);
            if (len > 0) {
                if (parse[0] == 's' && (i == 1) &&  ( parse[1] == '\n' || input[0] == '\r') ) { mode = 2; break; }

                if (isalpha(input[0]) || isspace(input[0])) {
                    parse[i] = input[0];
                    i++;
                }
                if (input[0] == '\n' || input[0] == '\r') {
                    parse[i] = '\0';
                    printf("\necho: %s\n", parse);
                    i = 0; parse[0] = '\0';
                }
            }
            uart_flush(EX_UART_NUM);
        }

        printf("hex to dec\n"); i = 0;
        while (mode == 2) {
            len = uart_read_bytes(EX_UART_NUM, input, BUF_SIZE, 20 / portTICK_RATE_MS);
            if (len>0) {
                if (isdigit(input[0]) && i < decimals) {
                    parse[i] = input[0];
                    parse[i+1] = '\0';
                    i++;
                }
                if (i == decimals || input[0] == '\n' || input[0] == '\r') {
                    int number = atoi(parse);
                    printf("\nhex: %X\n ", number);
                    i = 0; parse[0] = '\0';
                }
                else if (input[0] == 's') { mode = 0; break; }
            }
            uart_flush(EX_UART_NUM);
        }
    }
}
