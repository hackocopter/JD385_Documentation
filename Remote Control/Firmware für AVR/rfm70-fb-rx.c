#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "all-fb.h"
#include "../../uart.h"
#include "../rfm70.h"


#define UART_BAUD_RATE 9600

uint16_t            ticks;
volatile uint8_t    ticms;
PAYLOAD             pl;
uint8_t             hopping;
uint8_t             act_channel;
uint8_t             kanal[16];
uint8_t             cha_fine;
uint8_t             status;


//--------------------------------------
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


//--------------------------------------
void timer_init(void) {
    TCCR1B |= (1<<WGM12) | (1<<CS10 | 1<<CS12);
    TCNT1 = 0;
	OCR1A = ((F_CPU / 1024) / 1000) - 1;
	TIMSK1 |= (1 << OCIE1A);
    ticks = 0;
    ticms = 0;
    }


//--------------------------------------
ISR(TIMER1_COMPA_vect) {                // alle 1ms

    PORTA ^= 0x01;
    ++ticks;
    if (ticks >= 1000) {
        ticks = 0;
        PORTA ^= 0x80;
        }

    ticms++;
    }


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
void disp_abs(uint8_t x) {
    uart_putc('0' + (x / 100));
    x = x % 100;
    uart_putc('0' + (x / 10));
    uart_putc('0' + (x % 10));
    }


//--------------------------------------
uint8_t sum_pl() {

    uint8_t     i, sum;
    uint8_t     *ppl;

    ppl = (uint8_t *)&pl;
    sum = 0;
    for (i = 0; i < 14; i++) {
        sum += *ppl;
        ppl++;
        }
    return(sum & 0xff);
    }


//--------------------------------------
void help(void) {
    uart_puts_P("\nC = ++Hop-Kanal  c = --Hop-Kanal"
                "\nF = ++Kanal      f = --Kanal"
                "\nP = fester Kanal p = huepfen"
                "\n\n");
    }


//--------------------------------------
int main(void) {

    uint8_t         sum, hop_base, hop_inc;
    uint8_t         i, tmp;
    uint16_t        j;
    uint8_t         rx_ok;

    DDRA = 0xff;
    PORTA &= 0xf0;

    uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));
    timer_init();
    _delay_ms(100);
    sei();
    uart_puts_P("\033[2J\033[0;0H\n\n"
                "Fernsteuer Empfaenger (JD-385) Simulator\n\n"
                "RFM70 wird initialisiert...");
    if (RFM70_init()) {
        uart_puts_P("fehlgeschlagen, Abbruch\n");
        while(1);
        }
	setModeRX();
    uart_puts_P("fertig\n");

    status = BIND;
    help();
    while(1) {
        j = uart_getc();
        if ((j & 0xff00) == 0) {
            switch(j & 0xff) {
                case 'P':
                  hopping = 0;  
                  break;
                case 'p':
                  hopping = 1;  
                  break;
                case 'C':
                  act_channel = (act_channel + 1) & 0x0f;  
                  setChannel(kanal[act_channel]);
                  cha_fine = kanal[act_channel];
                  break;
                case 'c':
                  act_channel = (act_channel + 15) & 0x0f;  
                  setChannel(kanal[act_channel]);
                  cha_fine = kanal[act_channel];
                  break;
                case 'F':
                  cha_fine = (cha_fine + 1) & 0x7f;  
                  setChannel(cha_fine);
                  break;
                case 'f':
                  cha_fine = (cha_fine + 127) & 0x7f;  
                  setChannel(cha_fine);
                  break;
                default: help();
                }
            if (hopping) {
                uart_puts_P("\nhopping  ");
                }
            else {
                uart_puts_P("\nKanal ");
                disp_abs(cha_fine);
                }
            }
        PORTA ^= 0x02;
        i = receivePayload((uint8_t *)&pl);
        if (i != 0) {
            PORTA ^= 0x04;
            if ((status == BIND) && (pl.flags == 0xc0)) {
                status = RUN;
                uart_puts_P("\nSender gefunden. ID = ");
                disp_byte(pl.tx_id[0]);
                uart_putc('-');
                disp_byte(pl.tx_id[1]);
                uart_putc('-');
                disp_byte(pl.tx_id[2]);
                sum = pl.tx_id[0] + pl.tx_id[1] + pl.tx_id[2];
                hop_base = sum & 0x03;
                hop_inc = (sum & 0x1c) >> 2;
                uart_puts_P("\nKanaele: ");
                for (i = 0; i < 16; i++) {
                    tmp = pgm_read_byte(&hop_tbl[hop_base][i]) + hop_inc;
                    if ((tmp & 0x0f) == 0) tmp -=3;
                    kanal[i] = tmp;
                    disp_abs(tmp);
                    uart_putc(' ');
                    }
                uart_putc('\n');
                act_channel = 0;
                setChannel(kanal[act_channel]);
                cha_fine = kanal[act_channel];
                ticms = 0;
                rx_ok = 0;
                }
            if (status == RUN) {
                act_channel = (act_channel + 1) & 0x0f;  
                setChannel(kanal[act_channel]);
                cha_fine = kanal[act_channel];
                ticms = 0;
                rx_ok++;
                if (rx_ok == 0) rx_ok = 255;
                }
            }
        if (ticms == 127) {
            ticms = 0;
            disp_byte(rx_ok);
            rx_ok = 0;
            uart_putc('T');
            }
        }
    return(0);
    }


