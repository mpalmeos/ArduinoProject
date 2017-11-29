#define BLINK_DELAY_MS 500
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "uart.h"
#include "hmi_msg.h"
#include "print_helper.h"
#include "../lib/hd44780_111/hd44780.h"


static inline void init_leds(void)
{
    //Set port A pins 22, 24 and 26 as output.
    DDRA = 0x15;
    //Set port B pin 7 as output (onboard yellow LED)
    DDRB |= _BV(DDB7);
}


//Init console as stderr in UART1 and print user code info; init UART0 for later
static inline void init_con(void)
{
    simple_uart0_init();
    simple_uart1_init();
    stdout = stdin = &simple_uart0_io;
    stderr = &simple_uart1_out;
    /* Print version info to UART1 */
    fprintf_P(stderr, PSTR(CUR_VERSION), PSTR(FW_VERSION), PSTR(__DATE__),
              PSTR(__TIME__));
    fprintf_P(stderr, PSTR(AVR_VERSION), PSTR(__AVR_LIBC_VERSION_STRING__),
              PSTR(__VERSION__));
    /* Print student's name to UART0 */
    fprintf_P(stdout, PSTR(MY_NAME));
    fprintf(stdout, "\n");
}


static inline void blink_leds(void)
{
    /*Set port B pin 7 output to low to turn onboard LED off*/
    PORTB &= ~_BV(PORTB7);
    /*Set port A pin 22 to high to turn red LED on, wait and
    then turn it to low to turn it off*/
    PORTA |= _BV(PORTA0);
    _delay_ms(BLINK_DELAY_MS);
    PORTA &= ~ _BV(PORTA0);
    /*Set port A pin 22 to high to turn green LED on, wait and
    then turn it to low to turn it off*/
    PORTA |= _BV(PORTA2);
    _delay_ms(BLINK_DELAY_MS);
    PORTA &= ~ _BV(PORTA2);
    /*Set port A pin 22 to high to turn blue LED on, wait and
    then turn it to low to turn it off*/
    PORTA |= _BV(PORTA4);
    _delay_ms(BLINK_DELAY_MS);
    PORTA &= ~ _BV(PORTA4);
}


void main (void)
{
    /* Blink LEDs */
    blink_leds();
    /* Initialise LEDs */
    init_leds();
    /* Initialise console */
    init_con();
    /*Initialise LCD screen */
    lcd_init();
    /* Make sure LCD screen is clean */
    lcd_clrscr();
    /* Write student's name on LCD screen */
    lcd_puts_P(PSTR(MY_NAME));
    /* Get ASCII table values */
    unsigned char ascii[128] = {0};

    for (unsigned char i = 0; i < sizeof(ascii); i++) {
        ascii[i] = i;
    }

    /* Write out ACII table and, using the array, print ACII that is readable for humans */
    fprintf(stdout, "\n");
    print_ascii_tbl(stdout);
    fprintf(stdout, "\n");
    print_for_human(stdout, ascii, sizeof(ascii) - 1);

    while (1) {
        /* Print text, read input and print response to input */
        char input[20];
        fprintf_P(stdout, PSTR(GET_NUMBER_MSG));
        scanf("%s", input);
        fprintf(stdout, input);
        /* Turn input string into Integer (any non-digits will be read as zero) */
        int in_int = atoi(input);
        /* Go to next line on LCD screen */
        lcd_goto(0x40);

        /* Error Control: prints text from table, if Integer is between 0 and 9, and gives error, if number is higher/lower */
        if (in_int >= 0 && in_int < 10) {
            fprintf_P(stdout, PSTR(GIVE_NUMBER_MSG));
            fprintf_P(stdout, numbers[in_int]);
            lcd_puts_P(numbers[in_int]);
            lcd_putc(' ');
        } else {
            fprintf_P(stdout, PSTR(NOT_NUMBER_MSG));
        }

        lcd_puts_P(PSTR("                 ")); //Clear the end of the line on the screen
    }
}