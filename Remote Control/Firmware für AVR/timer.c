#include <avr/io.h>
#include <avr/interrupt.h>
#include "all-fb.h"
#include "../uart.h"
#include "vt100.h"
#include "rfm70.h"
#include "rfm70-fb.h"

// Interrupt alle 1ms

uint16_t    ticks;
PAYLOAD     pl;


//--------------------------------------
void timer_init(void) {
    TCCR1B |= (1<<WGM12) | (1<<CS10 | 1<<CS12);
    TCNT1 = 0;
	OCR1A = ((F_CPU / 1024) / 1000) - 1;
	TIMSK1 |= (1 << OCIE1A);
    ticks = 0;
    ticms = 0;
    timeout = 0;
    }


//--------------------------------------
ISR(TIMER1_COMPA_vect) {

    ++ticks;
    if (ticks >= 1000) {
        tic1s = 1;
        ticks = 0;
        }

    ticms++;
    }
