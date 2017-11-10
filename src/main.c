#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <assert.h>
//#include "uart.h"
#include "../lib/hd44780_111/hd44780.h"
#define _ASSERT_USE_STDERR
#define BLINK_DELAY_MS1 100
#define BLINK_DELAY_MS2 200
#define BLINK_DELAY_MS3 300


static inline void init_leds(void)
{
    DDRA=0x15;
    DDRB |= _BV(DDB7);
}

/*
static inline void init_errcon(void)
{
    simple_uart1_init();
    stderr = &simple_uart1_out;
    fprintf(stderr, "Version: %s built on: %s %s\n",
            FW_VERSION, _DATE_, _TIME_);
    fprintf(stderr, "avr-libc version: %s avr-gss version: %s\n",
            _AVR_LIBC_VERSION_STRING_, _VERSION_);
}
*/

static inline void blink_leds(void)
{
    PORTB &= ~_BV(PORTB7);

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


void main (void)
{
    init_leds();
    //init_errcon();
    lcd_init();
    lcd_home();
    lcd_puts("No entry - go get a key!");
    lcd_goto(LCD_ROW_2_START);

    char *array;
    uint32_t i = 1;
    extern int __heap_start, *__brkval;
    int v;
    array=malloc(i*sizeof(char));
    assert(array);


    while(1){
        blink_leds();
        array=realloc( array, (i++*100)*sizeof(char));
        fprintf(stderr, "%d\n",
                (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
        assert(array);
    }
}