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
volatile uint8_t    start_cnt;
volatile uint8_t    tick_slow;
volatile uint8_t    tick_fast;
uint8_t             ticms;
PAYLOAD             pl;
uint8_t             hopping;
uint8_t             act_channel;
uint8_t             kanal[16];
uint8_t             cha_fine;
uint8_t             stop;
volatile uint8_t    do_tx;
volatile uint8_t    do_hop;


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
	OCR1A = ((F_CPU / 1024) / 500) - 1;
	TIMSK1 |= (1 << OCIE1A);
    ticks = 0;
    ticms = 0;
    do_tx = 0;
    do_hop = 0;
    tick_slow = 0;
    tick_fast = 0;
    }


//--------------------------------------
ISR(TIMER1_COMPA_vect) {                // alle 2ms

    PORTD ^= 0x08;
    ++ticks;
    if ((ticks & 0x7f) == 0) tick_fast = 1;
    if ((ticks & 0x1ff) == 0) {
        tick_slow = 1;
        if (start_cnt != 0) start_cnt--;
        }

    ticms++;
    ticms = ticms & 0x03;
    switch (ticms) {
        case 0:
        case 2: do_tx = 1;
                break;
        case 3: do_hop = 1;
                break;
        }
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

    sum = 0;
    ppl = (uint8_t *) &pl;
    for (i = 0; i < 15; i++) {
        sum += *ppl;
        ppl++;
        }
    return(sum & 0xff);
    }


//--------------------------------------
void help(void) {
    uart_puts_P("\nG = ++Gas        g = --Gas"
                "\nH = ++Hoehe      h = --Hoehe"
                "\nS = ++Seite      s = --Seite"
                "\nQ = ++Quer       q = --Quer"
                "\nC = ++Hop-Kanal  c = --Hop-Kanal"
                "\nF = ++Kanal      f = --Kanal"
                "\nP = fester Kanal p = huepfen"
                "\nB = bind         b = Flug"
                "\nX = Stop         x = weiter"
                "\n\n");
    }


//--------------------------------------
void inc_p(uint8_t *p) {
    if (!(*p & 0x80)) {
        (*p)++;
        if (*p == 0x80) *p = 0x7f;
        }
    else {
        (*p)--;
        if (*p == 0x80) *p = 0;
        }
    }


//--------------------------------------
void dec_p(uint8_t *p) {
    if (!(*p & 0x80)) {
        (*p)--;
        if (*p == 0xff) *p = 0x81;
        }
    else {
        (*p)++;
        if (*p == 0x00) *p = 0xff;
        }
    }


//--------------------------------------
int main(void) {

    uint32_t        txid;
    uint8_t         sum, hop_base, hop_inc;
    uint8_t         i, tmp;
    uint16_t        j;
    uint8_t         *ppl;

    DDRD |= 0xf8;
    PORTD &= 0x07;
    DDRB |= 0x01;

    uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));
    timer_init();
    _delay_ms(100);
    sei();
    uart_puts_P("\033[2J\033[0;0H\n\n"
                "Fernsteuer Sender (JD-385) Simulator\n\n"
                "Seriennummer: ");
/*
    txid = 0;
    for (i = 0; i < 8; i++) {
        do {
            j = uart_getc();
            } while ((    (j & 0xff00) != 0)
                       || ((j & 0xff) < '0')
                       || ((j & 0xff) > '9')
                          );
        uart_putc(j & 0xff);
        txid = txid * 10;
        txid = txid + (j & 0x0f);
        }
*/
    txid = 0x123456;
    uart_puts_P("\nTx-Id: ");
    disp_byte((txid & 0xff0000) >> 16);
    uart_putc('-');
    disp_byte((txid & 0xff00) >> 8);
    uart_putc('-');
    disp_byte(txid & 0xff);
    uart_puts_P("\nSendekanaele: ");
    pl.tx_id[2] = txid & 0xff;
    pl.tx_id[1] = (txid & 0xff00) >> 8;
    pl.tx_id[0] = (txid & 0xff0000) >> 16;
    sum = pl.tx_id[0] + pl.tx_id[1] + pl.tx_id[2];
    hop_base = sum & 0x03;
    hop_inc = (sum & 0x1c) >> 2;
    for (i = 0; i < 16; i++) {
                tmp = pgm_read_byte(&hop_tbl[hop_base][i]) + hop_inc;
                if ((tmp & 0x0f) == 0) tmp -=3;
                kanal[i] = tmp;
                disp_abs(tmp);
                uart_putc(' ');
                }
    uart_putc('\n');
    act_channel = 0;
    hopping = 0;
    pl.throttle = 0;
    pl.rudder = 0;
    pl.elevator = 0;
    pl.aileron = 0;
    pl.trimm_yaw = 64;
    pl.trimm_pitch = 64;
    pl.trimm_roll = 64;
    for (i = 0; i < 4; i++) {
        pl.empty[i] = 0;
        }
    pl.flags = 0xc0;
    pl.chksum = sum_pl();

    uart_puts_P("\nRFM70 wird initialisiert...");
    if (RFM70_init()) {
        uart_puts_P("fehlgeschlagen, Abbruch\n");
        while(1) {
            PORTB |= 0x01;
            _delay_ms(200);
            PORTB &= 0xfe;
            _delay_ms(200);
            }
        }
    uart_puts_P("fertig\n");

	setModeTX();
    setChannel(kanal[act_channel]);
    cha_fine = kanal[act_channel];
    help();
    stop = 0;
    hopping = 1;
    start_cnt = 120;
    while(1) {
        j = uart_getc();
        if ((j & 0xff00) == 0) {
            switch(j & 0xff) {
                case 'G':
                  pl.throttle++;
                  if (pl.throttle == 0) pl.throttle = 255;  
                  break;
                case 'g':
                  pl.throttle--;  
                  if (pl.throttle == 255) pl.throttle = 0;  
                  break;
                case 'H':
                  inc_p(&pl.elevator);  
                  break;
                case 'h':
                  dec_p(&pl.elevator);  
                  break;
                case 'S':
                  inc_p(&pl.rudder);  
                  break;
                case 's':
                  dec_p(&pl.rudder);  
                  break;
                case 'Q':
                  inc_p(&pl.aileron);  
                  break;
                case 'q':
                  dec_p(&pl.aileron);  
                  break;
                case 'B':
                  pl.flags = 0xc0;  
                  break;
                case 'b':
                  pl.flags = 0x00;  
                  break;
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
                case 'X':
                  stop = 1;  
                  break;
                case 'x':
                  stop = 0;
                  break;
                default: help();
                }

            pl.chksum = sum_pl();
            if (hopping) {
                uart_puts_P("\nhopping  ");
                }
            else {
                uart_puts_P("\nKanal ");
                disp_abs(cha_fine);
                }
            uart_puts_P(" : ");
            ppl = (uint8_t *) &pl;
            for (i = 0; i < 16; i++) {
                disp_byte(*ppl);
                ppl++;
                uart_putc(' ');
                }
            }
        if (tick_slow) {
            tick_slow = 0;
            PORTB ^= 0x01;
            }
//        if (start_cnt == 0) pl.flags = 0;

//        if (!stop) {
            if (do_tx) {
                PORTD ^= 0x10;
                do_tx = 0;
                flushTxFIFO();
                if (!sendPayload((uint8_t *) &pl, 16, 0)) {
                    for (i = 0; i < 10; i++) {
                        PORTB ^= 0x01;
                        _delay_ms(200);
                        PORTB &= 0xfe;
                        _delay_ms(200);
                        }
                    }
                }
            if (do_hop && hopping) {
                do_hop = 0;
                PORTD ^= 0x20;
                act_channel = (act_channel + 1) & 0x0f;  
                setChannel(kanal[act_channel]);
                cha_fine = kanal[act_channel];
                }
//            }
        }
    return(0);
    }

