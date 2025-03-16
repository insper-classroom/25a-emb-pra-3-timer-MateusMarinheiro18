/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"


#define TRIG_PIN 15
#define ECHO_PIN 14

volatile bool alarm_flag = false;
volatile bool echo_flag = false; 

volatile long duration;

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    alarm_flag = true;
    return 0;
}

void echo_callback(uint gpio, uint32_t events) {
    static uint32_t start_time;
    static uint32_t end_time;

    if (events & GPIO_IRQ_EDGE_RISE) {
        start_time = time_us_32();
    } else if (events & GPIO_IRQ_EDGE_FALL) {
        end_time = time_us_32();
        duration = end_time - start_time;
        echo_flag = true;

    }
}

int main() {
    stdio_init_all();

    datetime_t t = {
        .year  = 2025,
        .month = 3,
        .day   = 16,
        .dotw  = 0, // 0 = domingo, 6 = sábado
        .hour  = 2,
        .min   = 10,
        .sec   = 0
    };
    rtc_init();
    rtc_set_datetime(&t);

    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN, 0);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    
    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &echo_callback);

    while (true) {
        gpio_put(TRIG_PIN, 1);
        sleep_ms(10);
        gpio_put(TRIG_PIN, 0);

        alarm_id_t alarm = add_alarm_in_us(50000, alarm_callback, NULL, true);

        if (alarm_flag) {
            alarm_flag = false;
            cancel_alarm(alarm);
            printf("Falha\n");
        }
        else if (echo_flag) {
            echo_flag = false;
            cancel_alarm(alarm);
            datetime_t now;
            rtc_get_datetime(&now);

            printf("[%04d-%02d-%02d %02d:%02d:%02d] Distância: %.2f cm\n", 
                now.year, now.month, now.day, now.hour, now.min, now.sec, 
                duration * 0.034 / 2);
        }

        sleep_ms(1000);

    }
}
