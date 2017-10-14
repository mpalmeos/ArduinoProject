#include <avr/io.h>
#include <util/delay.h>

#define BLINK_DELAY_MS 1000

void main (void) 
{
	DDRB |= _BV(DDB7);
	
	while(1) {
		PORTB |= _BV(PORTB7);
		_delay_ms(BLINK_DELAY_MS);
		PORTB &= ~_BV(PORTB7);
		_delay_ms(BLINK_DELAY_MS);
	}
}