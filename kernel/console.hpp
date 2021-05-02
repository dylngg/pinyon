#pragma once
#include <pine/string.hpp>

/*
 * For the Raspberry Pi 2 we are using the PL011 UART.
 *
 * See page 177 of Broadcom BCM2836 Datasheet for details
 * on these registers.
 */
#define GPIO14 14
#define GPIO15 15

#define UART_DATA 0x3F201000
#define UART_FR 0x3F201018
#define UART_IBRD 0x3F201024
#define UART_FBRD 0x3F201028

#define UART_LCRH 0x3F20102C
#define UART_LCRH_FEN 4
#define UART_LCRH_LWEN0 5
#define UART_LCRH_LWEN1 6

#define UART_CR 0x3F201030
#define UART_CR_TXE 8
#define UART_CR_RXE 9

#define UART_ICR 0x3F201044

#define UART_IMSC 0x3F201038
#define UART_IMSC_RIMIM 0
#define UART_IMSC_DCDMIM 2
#define UART_IMSC_DSRMIM 3

// See page 101 of Broadcom BCM2836 Datasheet
#define GPPUD 0x3F200094
#define GPPUDCLK0 0x3F200098

void console_init();

void console_readline(char*, size_t);

void console(const char*);

void console(const char*, size_t);

void consoleln(const char*);

void consolef(const char*, ...) __attribute__((format(printf, 1, 2)));