// Console I/O (using UART)
// Author: Shazor Shahid
// Date: 9/12/2020
// Source code used: UART echo example

#include <stdio.h>
#include <ctype.h>
#include "driver/uart.h"
#include "driver/gpio.h"

/**
 * This is an example which echos any data it receives on UART0 back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: UART0
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: use the default pins rather than making changes
 */

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
    RESET(PIN);
    gpio_set_direction(PIN, GPIO_MODE_OUTPUT);

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

    // Configure a temporary buffer for the incoming data
    // uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    char        input[BUF_SIZE];
    char        parse[BUF_SIZE];
    int         i           = 0;
    int         mode        = 0;    // 0 = toggle mode, 1 = echo mode, 2 = int-hex mode
    bool        ledOnOff    = 0;    // 0 = off, 1 = on
    const int   decimals    = 2;

    printf("toggle mode\n");

    while (1) {
        // Read data from the UART
        // int len = uart_read_bytes(EX_UART_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        // Write data back to the UART
        // if (len > 0) { printf("Read: %c \n", *data); }
        int len = uart_read_bytes(EX_UART_NUM, input, BUF_SIZE, 20 / portTICK_RATE_MS);
        if (len > 0) { printf("Read %c\n", input[0]); }

        printf("toggle mode\n");
        while (mode == 0) {
            len = uart_read_bytes(EX_UART_NUM, input, BUF_SIZE, 20 / portTICK_RATE_MS);
            if (len > 0) {
                if (input[0] == 's') { mode = 1; break; }

                else if (input[0] == 't') {
                    printf("read: %c", input[0]);
                    ledOnOff = !ledOnOff;
                    if (ledOnOff) { ON(PIN); }
                    else if (!ledOnOff) { OFF(PIN); }
                }
            }
        }

        printf("echo mode\n");
        while (mode == 1) {
            len = uart_read_bytes(EX_UART_NUM, input, BUF_SIZE, 20 / portTICK_RATE_MS);
            if (len>0) {

                if (input[0] == 's') { mode = 2; break; }

                else if (input[0] != '\n') {
                    parse[i] = input[0];    // if valid characters, add to new array to store
                    printf("%c ", parse[i]);
                    i++;                    // increment place in array
                }
                else if (input[0] == '\n') {
                    // print out current array
                    printf("echo: ");
                    for (int j = 0; j < i; j++) { printf("%c", parse[i]); }
                    i = 0; // reset i
                }
            }
        }
        printf("hex to dec\n");
        while (mode == 2) {
            len = uart_read_bytes(EX_UART_NUM, input, BUF_SIZE, 20 / portTICK_RATE_MS);
            if (len>0) {
                i = 0;
                if (input[0] == 's') { mode = 0; break; }
                else if (isdigit(input[0])) {
                    i++;
                    parse[i] = input[0];
                    if (i==decimals) {
                        int newInt = 0;
                        for (int j = 0; j < decimals; j++ ) {
                            newInt += parse[j];
                        }
                        printf("hex: %X\n", newInt);
                    }
                }
            }
        }
/*

        if (len > 0 && mode == 1) {
            if (input[0] != '\n') { 
                parse[i] = input[0]; i++; printf("%c",parse[i]);
            }
            else if (input[0] == '\n') {
                for (int j = 0; j<i; j++) { 
                    printf("%c", parse[j]); 
                }
                i = 0;
            }
            if (input[0] == 's') { 
                mode++; printf("dec to hex mode\n"); 
            }
        }

*/
    }
}
