// VT100 Escape Sequenzen (nur ein kleiner Teil)

#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "../uart.h"

#define t_putc  uart_putc

void send_esc(uint8_t c) {
    t_putc(0x1b);
    t_putc(c);
    }


void send_dec(uint8_t n) {
    t_putc('0' + (n/10));
    t_putc('0' + (n%10));
    }


void term_clear(void) {
    send_esc('[');
    t_putc('2');
    t_putc('J');
    }


void term_cursor(uint8_t row, uint8_t column) {
    send_esc('[');
    send_dec(row);
    t_putc(';');
    send_dec(column);
    t_putc('H');
    }



