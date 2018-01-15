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
#include "../lib/andy_brown_memdebug/memdebug.h"
#include "../lib/matejx_avr_lib/mfrc522.h"
#include "rfid.h"

#define UART_BAUD 9600
#define UART_STATUS_MASK 0x00FF

//#define BLINK_DELAY_MS 2000
#define LED_RED PORTA0 //Arduino Mega digital pin 22 for heartbeat
#define LED_BLU PORTA4 //Arduino Mega digital pin 26 for door lock

volatile uint32_t counter_1; // Global seconds counter

// Create microrl object and pointer on it
static microrl_t rl;
static microrl_t *prl = &rl;


static inline void init_leds(void)
{
    //Set pin of PORTA for output and set to low
    DDRA |= _BV(LED_RED);
    DDRA |= _BV(LED_BLU);
    PORTA &= ~_BV(LED_RED);
    PORTA &= ~_BV(LED_BLU);
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
    TCCR1B |= _BV(CS12); // fCPU/256
    OCR1A = 62549; // 1 s. Note that it is actually two registers OCR5AH and OCR5AL
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


static inline void init_rfid_reader(void)
{
    /* Init RFID-RC522 */
    MFRC522_init();
    PCD_Init();
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


static inline uint32_t get_time(void)
{
    uint8_t time_now = 0;
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        time_now = counter_1;
    }
    return time_now;
}


static inline void heartbeat(void)
{
    static uint32_t prev_time;
    uint32_t now = get_time();
    char print_buf[11] = {0x00}; // Buffer large enough to hold all long (uint32_t) digits

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


void door_and_disp_handler(void)
{
    door_state_t door_state;
    display_state_t display_state;
    Uid uid;
    Uid *uid_ptr = &uid;
    uint8_t current_time = get_time();
    card_t *card_beep;
    static uint32_t message_start;
    static uint32_t door_open_start;
    char lcd_buf[256];
    byte bufferATQA[10];
    byte buffersize = sizeof(bufferATQA);
    byte result;
    
    if (PICC_IsNewCardPresent()) {
        result = PICC_WakeupA(bufferATQA, &buffersize);
        if (result != STATUS_OK){
        PICC_ReadCardSerial(uid_ptr);
        card_beep->size = uid.size;
        memcpy(card_beep->uid, uid.uidByte, uid.size);
        //card_beep->holder = NULL;
        card_t *card_found = rfid_card_finder(card_beep);

        if (card_found) {
            door_state = door_opening;
            display_state = display_name;
        } else {
            door_state = door_closed;
            display_state = display_access_denied;
        }
        }
    } else {
        return;
    }

    switch (door_state) {
    case door_opening:
        //Document door open time
        door_open_start = current_time;
        //Unlock door
        door_state = door_open;
        break;

    case door_open:
        if ((door_open_start + 2) < current_time) {
            door_state = door_closing;
        }

        //Open door
        PORTA |= _BV(LED_BLU);
        break;

    case door_closing:
        //Lock door
        door_state = door_closed;
        display_state = display_clear;
        PORTA &= _BV(LED_BLU);
        break;

    case door_closed:
        break; // No need to do anything
    }

    switch (display_state) {
    case display_access_denied:
        lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
        lcd_goto(LCD_ROW_2_START);
        lcd_puts_P(PSTR("Access denied!"));
        message_start = current_time;
        display_state = display_clear;
        break;

    case display_clear:
        if ((message_start + 5) < current_time) {
            lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
            display_state = display_no_update;
        }
        lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
        break;

    case display_name:
        lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
        lcd_goto(LCD_ROW_2_START);

        if (card_beep->holder != NULL) {
            strncpy(lcd_buf, card_beep->holder, LCD_VISIBLE_COLS);
            lcd_puts(lcd_buf);
        } else {
            lcd_puts_P(PSTR("Name read error"));
        }

        message_start = current_time;
        display_state = display_clear;
        break;

    case display_no_update:
        break;
    }
}


void main (void)
{
    /* Initialise console, microrl and print start info */
    init_con();
    print_start_msg();
    microrl_initialize();
    init_rfid_reader();

    while (1) {
        //Print Uptime in UART0
        heartbeat();
        //Get input from user via commandline and execute the commands
        microrl_insert_char(prl, (uart0_getc() & UART_STATUS_MASK));
        door_and_disp_handler();
    }
}

/* Counter 1 ISR */
ISR(TIMER1_COMPA_vect)
{
    counter_1++;
}