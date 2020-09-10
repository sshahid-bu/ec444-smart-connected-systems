// Name:        LED Binary Counter
// Author:      Shazor Shahid sshahid@bu.edu
// Assignment:  EC444 Quest 1 Skill 07
// 

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define N1 26 // A0
#define N2 25 // A1
#define N3 27 // 27
#define N4 21 // 21

#define RESET(x)    gpio_reset_pin(x);
#define ON(x)       gpio_set_level(x, 1)
#define OFF(x)      gpio_set_level(x, 0)
#define PAUSE       1000 / portTICK_PERIOD_MS

void app_main(void) {

    RESET(N1); 
    RESET(N2); 
    RESET(N3); 
    RESET(N4);
    gpio_set_direction(N1, GPIO_MODE_OUTPUT); 
    gpio_set_direction(N2, GPIO_MODE_OUTPUT);
    gpio_set_direction(N3, GPIO_MODE_OUTPUT); 
    gpio_set_direction(N4, GPIO_MODE_OUTPUT);
    
    int num = 0; 
    
    while(1) {
        if (num >= 16) { 
            printf("reset\n");
            num = 0; 
            OFF(N1); 
            OFF(N2); 
            OFF(N3); 
            OFF(N4);
            vTaskDelay(PAUSE);
        } 
        if (num == 0) {
            printf("num: 0000\n");
            OFF(N1); 
            OFF(N2); 
            OFF(N3); 
            OFF(N4);
            vTaskDelay(PAUSE);
        }
        
        num = num + 1;

        while(num < 16) {
            OFF(N1); 
            OFF(N2); 
            OFF(N3); 
            OFF(N4);
            switch(num) {
                case 1:
                    ON(N1);
                    printf("num: 0001\n"); 
                    vTaskDelay(PAUSE); 
                    break;
                case 2:
                    ON(N2);
                    printf("num: 0010\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 3:
                    ON(N1); 
                    ON(N2);
                    printf("num: 0011\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 4:
                    ON(N3); 
                    printf("num: 0100\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 5:
                    ON(N3); 
                    ON(N1);
                    printf("num: 0101\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 6:
                    ON(N3); 
                    ON(N2);
                    printf("num: 0110\n");
                    vTaskDelay(PAUSE);
                    break;
                case 7:
                    ON(N3); 
                    ON(N2); 
                    ON(N1);
                    printf("num: 0111\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 8: 
                    ON(N4);
                    printf("num: 1000\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 9:
                    ON(N4); 
                    ON(N1);
                    printf("num: 1001\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 10:
                    ON(N4); 
                    ON(N2);
                    printf("num: 1010\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 11:
                    ON(N4); 
                    ON(N2); 
                    ON(N1);
                    printf("num: 1011\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 12:
                    ON(N4); 
                    ON(N3);
                    printf("num: 1100\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 13:
                    ON(N4); 
                    ON(N3); 
                    ON(N1);
                    printf("num: 1101\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 14:
                    ON(N4); 
                    ON(N3); 
                    ON(N2);
                    printf("num: 1110\n"); 
                    vTaskDelay(PAUSE);
                    break;
                case 15:
                    ON(N4); 
                    ON(N3); 
                    ON(N2); 
                    ON(N1);
                    printf("num: 1111\n"); 
                    vTaskDelay(PAUSE);
                    break;
            } break;
        
        }

        /*
        gpio_set_level(N1, 1);
        gpio_set_level(N2, 1);
        gpio_set_level(N3, 1);
        gpio_set_level(N4, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        printf("Turning off the LED\n");
        gpio_set_level(N1, 0);
        gpio_set_level(N2, 0);
        gpio_set_level(N3, 0);
        gpio_set_level(N4, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        */
    }

}
