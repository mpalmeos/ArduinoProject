#include <avr/io.h>
#include <util/delay.h>
#define BLINK_DELAY_MS1 500
#define BLINK_DELAY_MS2 1000
#define BLINK_DELAY_MS3 1500

void main (void){
    
    DDRA=0x15;

    while(1){
        PORTA |= _BV(PORTA0);
        _delay_ms(BLINK_DELAY_MS1);
        PORTA &= ~ _BV(PORTA0);

        PORTA |= _BV(PORTA2);
        _delay_ms(BLINK_DELAY_MS2);
        PORTA &= ~ _BV(PORTA2);

        PORTA |= _BV(PORTA4);
        _delay_ms(BLINK_DELAY_MS3);
        PORTA &= ~ _BV(PORTA4);
    }
}