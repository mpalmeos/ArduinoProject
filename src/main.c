#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h> // stdlib is needed to use ltoa() - Long to ASCII
#include "hmi_msg.h"
#include "print_helper.h"
#include "cli_microrl.h"
#include "../lib/hd44780_111/hd44780.h"
#include "../lib/andygock_avr-uart/uart.h"
#include "../lib/helius_microrl/microrl.h"

#define UART_BAUD 9600
#define UART_STATUS_MASK 0x00FF

#define BLINK_DELAY_MS 900
#define LED_RED PORTA0 //Arduino Mega digital pin 22

/* Define counter scale here */
#define COUNT_SECONDS // seconds

/* Uncomment to get ASCII print. Otherwise bytes are sent */
#define ASCII_PRINT
#ifdef ASCII_PRINT
#endif //ASCII_PRINT

volatile uint32_t counter_1; // Global seconds counter
uint32_t prev_time = 0; // Heartbeat place holders
uint32_t now = 0;

// Create microrl object and pointer on it
static microrl_t rl;
static microrl_t *prl = &rl;


static inline void init_leds(void)
{
    //Set pin 22 of PORTA for output and set to low
    DDRA |= _BV(LED_RED);
    PORTA &= ~_BV(LED_RED);
}


static inline void init_sys_timer(void)
{
    // Set counter to random number 0x19D5F539 in HEX. Set it to 0 if you want
    counter_1 = 0;
    //Clear control registers
    TCCR1A = 0;
    TCCR1B = 0;
    // Turn on CTC (Clear Timer on Compare)
    TCCR1B |= _BV(WGM12);
#ifdef COUNT_SECONDS
    TCCR1B |= _BV(CS12); // fCPU/256
    OCR1A = 62549; // 1 s. Note that it is actually two registers OCR5AH and OCR5AL
#endif // COUNT_SECONDS
    // Output Compare A Match Interrupt Enable
    TIMSK1 |= _BV(OCIE1A);
}


// Initalise LCD screen and make sure it is empty
static inline void init_lcd(void)
{
    lcd_init();
    lcd_clrscr();
}


//Init system timer, LED pin, UART0, UART1, LCD and allow interruptions
static inline void init_con(void)
{
    //Start system timer
    init_sys_timer();
    //Initialize LED
    init_leds();
    //Initialize and define UARTs
    uart0_init(UART_BAUD_SELECT(UART_BAUD, F_CPU));
    uart1_init(UART_BAUD_SELECT(UART_BAUD, F_CPU));
    //Initialize LCD screen
    init_lcd();
    //Enable the use of interrupts
    sei();
}


static inline void print_start_msg(void)
{
    /* Print message and version info to UART1 */
    uart1_puts_p(PSTR("Console started\r\n"));
    uart1_puts_p(PSTR(VER_FW));
    uart1_puts_p(PSTR(VER_LIBC));
    /* Print student's name to UART0 */
    uart0_puts_p(PSTR(MY_NAME "\r\n"));
    uart0_puts_p(PSTR("Use backspace to delete entry and enter to confirm.\r\n"));
    uart0_puts_p(PSTR("Arrow keys and del doesn't work currently.\r\n"));
    /* Write student's name on LCD screen */
    lcd_puts_P(PSTR(MY_NAME));
}


static inline void heartbeat(void)
{
#ifdef ASCII_PRINT
    char print_buf[11] = {0x00}; // Buffer large enough to hold all long (uint32_t) digits
#endif //ASCII_PRINT
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        now = counter_1;
    }

    if (prev_time != now) {
        ltoa(now, print_buf, 10); //convert integer to string
        uart1_puts_p(PSTR("Uptime: "));
        uart1_puts(print_buf);
        uart1_puts_p(PSTR(" s.\r\n"));
        PORTA ^= _BV(LED_RED); //Toggle LED
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            prev_time = now;
        }
    }
}


static inline void microrl_initialize(void)
{
    //Call init with ptr to microrl instance and print callback
    microrl_init(prl, uart0_puts);
    //Set callback for execute
    microrl_set_execute_callback(prl, cli_execute);
}

void main (void)
{
    /* Initialise console, microrl and print start info */
    init_con();
    microrl_initialize();
    print_start_msg();

    while (1) {
        //Print Uptime in UART0
        heartbeat();
        //Get input from user via commandline and execute the commands
        microrl_insert_char(prl, (uart0_getc() & UART_STATUS_MASK));
    }
}

/* Counter 1 ISR */
ISR(TIMER1_COMPA_vect)
{
    counter_1++;
}