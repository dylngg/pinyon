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

#define UART_BASE 0x3F201000

#define UART_LCRH_FEN 4
#define UART_LCRH_LWEN0 5
#define UART_LCRH_LWEN1 6

#define UART_CR_EN 0
#define UART_CR_TXE 8
#define UART_CR_RXE 9

// See page 101 of Broadcom BCM2836 Datasheet
#define GPPUD 0x3F200094
#define GPPUDCLK0 0x3F200098

void console_readline(char*, size_t);

void console(const char*);

void console(const char*, size_t);

void consoleln(const char*);

void consolef(const char*, ...) __attribute__((format(printf, 1, 2)));

struct UARTManager {
public:
    void init() volatile;
    inline char get() volatile;
    inline void put(char ch) volatile;

private:
    /*
     * See section 13.3 on page 176 in the BCM2835 manual for the
     * definition of these.
     *
     * Most are not used here and not all registers are supported
     * by broadcom (since the BCM2835's UART is based on PrimeCell's
     * PL011).
     *
     * See also the PrimeCell UART (PL011) Manual:
     * https://developer.arm.com/documentation/ddi0183/f/
     */
    volatile u32 data; // DR
    volatile u32 recv_status; // RSRECR
    volatile u32 unused1;
    volatile u32 unused2;
    volatile u32 unused3;
    volatile u32 unused4;
    volatile u32 flag; // FR
    volatile u32 ibrd; // IBPR
    volatile u32 ilrd; // ILPR
    volatile u32 int_baud_rate_div; // IBRD
    volatile u32 frac_baud_rate_div; // FBRD
    volatile u32 line_ctrl; // LCRH
    volatile u32 ctrl; // CR
    volatile u32 intr_fifo_select; // IFLS
    volatile u32 intr_mask_set_clear; // IMSC
    volatile u32 raw_intr_status; // RIS
    volatile u32 masked_intr_status; // MIS
    volatile u32 intr_clear; // ICR
    volatile u32 dma_ctrl; // DMACR
};

inline UARTManager* uart_manager();

void uart_init();