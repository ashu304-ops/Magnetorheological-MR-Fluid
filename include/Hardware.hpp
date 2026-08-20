#pragma once

#include <cstdint>


#ifdef __cplusplus
extern "C" {
#endif


void hw_init(void);

void hw_gpio_init(void);

void hw_uart_puts(
    const char* str
);

void hw_led_toggle(void);

uint8_t hw_led_get_state(void);


#ifdef __cplusplus
}
#endif