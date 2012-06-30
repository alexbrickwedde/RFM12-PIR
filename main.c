#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "rf12.h"
#include "main.h"

#include <util/delay.h>

#define AIRID "0201"

volatile unsigned long PIR = 0;

void send() {
	wdt_reset();
	rf12_init();
	rf12_setfreq(RF12FREQ868(868.3));
	rf12_setbandwidth(4, 1, 4);
	rf12_setbaud(666);
	rf12_setpower(0, 6);

	unsigned long temp;
	wdt_reset();
	if (PIR != 0) {
		LED_AN(LED1);
		LED_AN(LED2);
		temp = 0xaaaa;
	} else {
		LED_AUS(LED1);
		LED_AUS(LED2);
		temp = 0;
	}

	char id[8];
	char test[32];
	test[0] = 'M';
	memcpy(test + 1, &(temp), 2);
	test[3] = 'M';
	memcpy(test + 4, &(temp), 2);

	rf12_txdata(test, 22);
	wdt_reset();
	rf12_trans(0x8201);
	wdt_reset();
	rf12_trans(0x0);
}

#define MAXCOUNT 300
unsigned int volatile WDTcounter = MAXCOUNT - 1;

ISR(WDT_OVERFLOW_vect)
{
	cli();
	WDTCSR |= _BV(WDIE) | _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
	WDTcounter++;
	sei();
}

ISR(INT0_vect)
{
	cli();
	WDTCSR |= _BV(WDIE) | _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
	if ((PIND & (1 << PD2)) == 0) {
		PIR = 0;
		LED_AUS(LED1);
		LED_AUS(LED2);
	} else {
		LED_AN(LED1);
		LED_AN(LED2);
		PIR = 1;
	}
	WDTcounter = MAXCOUNT;
	sei();
}

int main(void) {

	for (int i = 0; i < 10; i++) {
		_delay_ms(200);
		LED_AN(LED1);
		LED_AN(LED2);
		_delay_ms(200);
		LED_AUS(LED1);
		LED_AUS(LED2);
	}

	cli();
	wdt_reset();
	wdt_enable (WDTO_1S);
	WDTCSR |= _BV(WDIE) | _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
	sei();

	DDRD |= (1 << PD4);
	DDRD |= (1 << PD5);

	MCUCR = (1 << ISC00);
	GIMSK |= (1 << INT0);

	rf12_preinit(AIRID);

	if ((PIND & (1 << PD2)) == 0) {
		PIR = 0;
		LED_AUS(LED1);
		LED_AUS(LED2);
	} else {
		LED_AN(LED1);
		LED_AN(LED2);
		PIR = 1;
	}

	for (;;) {

		wdt_reset();

		if (WDTcounter >= MAXCOUNT) {
			send();
			WDTcounter = 0;
		}
		wdt_reset();

		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();
		sleep_cpu ();
		sleep_disable ();
	}
}
