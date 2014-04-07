#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "all-fb.h"
#include "../uart.h"
#include "rfm70.h"
#include "vt100.h"
#include "timer.h"


#define UART_BAUD_RATE 9600


const uint8_t PROGMEM hop_tbl[4][16] = {
 { 0x27, 0x1B, 0x39, 0x28, 0x24, 0x22, 0x2E, 0x36,
   0x19, 0x21, 0x29, 0x14, 0x1E, 0x12, 0x2D, 0x18 }, //  00
 { 0x2E, 0x33, 0x25, 0x38, 0x19, 0x12, 0x18, 0x16,
   0x2A, 0x1C, 0x1F, 0x37, 0x2F, 0x23, 0x34, 0x10 }, //  01
 { 0x11, 0x1A, 0x35, 0x24, 0x28, 0x18, 0x25, 0x2A,
   0x32, 0x2C, 0x14, 0x27, 0x36, 0x34, 0x1C, 0x17 }, //  02
 { 0x22, 0x27, 0x17, 0x39, 0x34, 0x28, 0x2B, 0x1D,
   0x18, 0x2A, 0x21, 0x38, 0x10, 0x26, 0x20, 0x1F }  //  03
 };

uint8_t     rx_channel[16];
uint8_t     act_channel;
uint8_t     our_txid[3];
PAYLOAD     pload;
uint8_t     ticms;
uint8_t     tic1s;
uint8_t     timeout;
uint8_t     status;


const char PROGMEM status_str[2][5] = {
                {"bind"},
                {"run "}
                };


//--------------------------------------
const char PROGMEM maske[] = {
    "JD-385 Monitor\n\n"
    "Tx-ID: 00 00 00\n\n"
    "              Poti   Trimm\n"
    "Gas         :  000\n"
    "Seitenruder :  000 -  000\n"
    "Hoehenruder :  000 -  000\n"
    "Querruder   :  000 -  000\n\n"
    "Flags       :   00\n"
    "Status      :  "
    };


//--------------------------------------
char hex_str[] = "0123456789ABCDEF";
char to_hex(uint8_t nibble) {
    return(hex_str[nibble & 0x0f]);
    }


//--------------------------------------
void disp_byte(uint8_t x) {
    uart_putc(to_hex(x >> 4));
    uart_putc(to_hex(x));
    }


//--------------------------------------
void disp_dec(int8_t x) {
    if (x & 0x80) {
        uart_putc('-');
        x = x & 0x7f;
        }
    else {
        uart_putc(' ');
        }
    uart_putc('0' + (x / 100));
    x = x % 100;
    uart_putc('0' + (x / 10));
    uart_putc('0' + (x % 10));
    }


//--------------------------------------
void disp_trimm(int8_t x) {
    if (x < 0) {
        uart_putc('-');
        x = abs(x);
        }
    else {
        uart_putc(' ');
        }
    uart_putc('0' + (x / 100));
    x = x % 100;
    uart_putc('0' + (x / 10));
    uart_putc('0' + (x % 10));
    }


//--------------------------------------
void disp_abs(uint8_t x) {
    uart_putc('0' + (x / 100));
    x = x % 100;
    uart_putc('0' + (x / 10));
    uart_putc('0' + (x % 10));
    }


//--------------------------------------
void update_data(void) {

    term_cursor(6, 16);
    disp_abs(pload.throttle);

    term_cursor(7, 15);
    disp_dec(pload.rudder);
    uart_puts_P("   ");
    disp_trimm(pload.trimm_yaw -0x40);

    term_cursor(8, 15);
    disp_dec(pload.elevator);
    uart_puts_P("   ");
    disp_trimm(pload.trimm_pitch -0x40);

    term_cursor(9, 15);
    disp_dec(pload.aileron);
    uart_puts_P("   ");
    disp_trimm(pload.trimm_roll -0x40);

    term_cursor(11, 17);
    disp_byte(pload.flags);

    term_cursor(12, 16);
    uart_puts_p(status_str[status]);
    }


//--------------------------------------
int main(void) {



    DDRA |= 0x03;
    DDRD |= 0x0c;
    uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));
    _delay_ms(100);
    sei();
    term_clear();
    term_cursor(0, 0);
    uart_puts_P("\n\nFernsteuerung Rx Test RFM70\n");

    uart_puts_P("RFM70 wird initialisiert...");
    if (RFM70_init()) {
        uart_puts_P("fehlgeschlagen, Abbruch\n");
        while(1);
        }
    uart_puts_P("fertig\n");

	setModeRX();
	setChannel(8);
    _delay_ms(1000);
    term_clear();
    term_cursor(0, 0);
    uart_puts_p(maske);
    status = BIND;
    tic1s = 0;
    timer_init();
    sei();
    while(1) {
        if ((volatile uint8_t) tic1s) {
            update_data();
            tic1s = 0;
            if (status == BIND) PORTD ^= 0x04;
            }

        if (receivePayload((uint8_t *) &pload))	{
            PORTA ^= 0x01;
            }
        _delay_us(1000);

        if ((status == BIND) && (pload.flags == 0xc0)) {
            memcpy(our_txid, pload.tx_id, 3);
            term_cursor(3, 8);
            disp_byte(our_txid[0]);
            uart_putc(' ');
            disp_byte(our_txid[1]);
            uart_putc(' ');
            disp_byte(our_txid[2]);
            status = RUN;
            }
        }
    }




