#include <stdint.h>

/* ============================================================
 * STM32F4 USART1
 *
 * USART1 base = 0x40011000
 * ============================================================ */

#define USART1_BASE      0x40011000UL

#define USART1_SR        (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR        (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR       (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1       (*(volatile uint32_t *)(USART1_BASE + 0x0C))

/* RCC */

#define RCC_BASE         0x40023800UL

#define RCC_AHB1ENR      (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB2ENR      (*(volatile uint32_t *)(RCC_BASE + 0x44))

/* GPIOA */

#define GPIOA_BASE       0x40020000UL

#define GPIOA_MODER      (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRH       (*(volatile uint32_t *)(GPIOA_BASE + 0x24))

/* ============================================================
 * USART1 initialization
 *
 * Assumes approximately 16 MHz peripheral clock.
 * 115200 baud:
 *
 * BRR ≈ 16000000 / 115200 = 139
 * ============================================================ */

void hw_uart_init(void)
{
    /* Enable GPIOA clock */
    RCC_AHB1ENR |= (1U << 0);

    /* Enable USART1 clock */
    RCC_APB2ENR |= (1U << 4);

    /*
     * PA9 = USART1_TX
     * PA10 = USART1_RX
     *
     * Alternate function mode = 10
     */

    GPIOA_MODER &= ~(3U << (9 * 2));
    GPIOA_MODER &= ~(3U << (10 * 2));

    GPIOA_MODER |= (2U << (9 * 2));
    GPIOA_MODER |= (2U << (10 * 2));

    /* AF7 = USART1 */
    GPIOA_AFRH &= ~(0xFU << ((9 - 8) * 4));
    GPIOA_AFRH &= ~(0xFU << ((10 - 8) * 4));

    GPIOA_AFRH |= (7U << ((9 - 8) * 4));
    GPIOA_AFRH |= (7U << ((10 - 8) * 4));

    /*
     * Baud rate.
     *
     * QEMU is tolerant for this simple test.
     */
    USART1_BRR = 139U;

    /*
     * UE  = USART enable
     * TE  = transmitter enable
     * RE  = receiver enable
     */

    USART1_CR1 =
        (1U << 13) |
        (1U << 3)  |
        (1U << 2);
}


/* ============================================================
 * Send one character
 * ============================================================ */

void hw_uart_putc(char c)
{
    /* Wait until TXE */
    while ((USART1_SR & (1U << 7)) == 0U)
    {
    }

    USART1_DR = (uint32_t)c;
}


/* ============================================================
 * Send string
 * ============================================================ */

void hw_uart_puts(const char* str)
{
    while (*str)
    {
        hw_uart_putc(*str++);
    }
}


/* ============================================================
 * Hardware initialization
 * ============================================================ */

void hw_init(void)
{
    hw_uart_init();

    hw_uart_puts("\r\n");
    hw_uart_puts("==============================\r\n");
    hw_uart_puts("STM32F4 QEMU START\r\n");
    hw_uart_puts("==============================\r\n");
}


/* ============================================================
 * GPIO / LED test
 *
 * QEMU STM32 machine does not model GPIO according to
 * current QEMU documentation, so keep this as a software
 * state for now.
 * ============================================================ */

static volatile uint8_t g_led_state = 0;

void hw_gpio_init(void)
{
    g_led_state = 0;

    hw_uart_puts("[HW] GPIO initialized\r\n");
}


void hw_led_toggle(void)
{
    g_led_state ^= 1U;

    if (g_led_state)
    {
        hw_uart_puts("[HW] LED ON\r\n");
    }
    else
    {
        hw_uart_puts("[HW] LED OFF\r\n");
    }
}


uint8_t hw_led_get_state(void)
{
    return g_led_state;
}