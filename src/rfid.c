#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "../lib/hd44780_111/hd44780.h"
#include "../lib/andygock_avr-uart/uart.h"
#include "../lib/helius_microrl/microrl.h"
#include "../lib/matejx_avr_lib/mfrc522.h"
#include "hmi_msg.h"
#include "print_helper.h"
#include "cli_microrl.h"
#include "rfid.h"

typedef struct card {
    uint8_t *uid;
    int size;
    char *holder;
    struct card *next;
} card_t;

typedef enum {
    door_opening,
    door_open,
    door_closing,
    door_closed,
} door_state_t;

typedef enum {
    display_name,
    display_access_denied,
    display_clear,
    display_no_update,
} display_state_t;

static card_t *card_list_head = NULL;
char *display_name_str;
char *prev_display_name;
door_state_t door_state = door_closed;
display_state_t display_state = display_no_update;
display_state_t prev_displ_state;
time_t time_y2k_cpy, door_open_time, msg_display_time, card_read_time, now_time;
bool match = false;

#define DOOR_OPEN_IN_SEC 2
#define DISPLAY_MSG_IN_SEC 3
#define LED_BLU PORTA4

//Read card data and print in console
void cli_rfid_read(const char *const *argv)
{
    (void) argv;
    Uid uid;
    Uid *uid_ptr = &uid;
    byte bufferATQA[10];
    byte buffersize = sizeof(bufferATQA);
    byte result = PICC_WakeupA(bufferATQA, &buffersize);

    if (result == STATUS_OK || result == STATUS_COLLISION) {
        uart0_puts_p(PSTR("Card selected!\r\n"));
        PICC_ReadCardSerial(uid_ptr);
        uart0_puts_p(PSTR("Card type: "));
        uart0_puts(PICC_GetTypeName(PICC_GetType(uid.sak)));
        uart0_puts_p(PSTR("\r\n"));
        uart0_puts_p(PSTR("Card UID: "));
        print_bytes_for_human(uid.uidByte, uid.size);
        uart0_puts_p(PSTR(" (size "));
        print_bytes_for_human(&uid.size, 1);
        uart0_puts_p(PSTR(" bytes)"));
        uart0_puts_p(PSTR("\r\n"));
        free(uid_ptr);
    } else {
        uart0_puts_p(PSTR("Unable to select card!\r\n"));
    }
}

//Add card to list
void cli_rfid_add(const char *const *argv)
{
    int uid_char_size = strlen(argv[1]);
    //Get card size (in HEX each 2 characters are 1 byte)
    uint8_t uid_size[strlen(argv[1]) / 2];
    char print_buf[256] = {0x00};

    //Check UID length
    if (uid_char_size > 20) {
        uart0_puts_p(PSTR("UID is too long! Max length is 10 bytes.\r\n"));
        return;
    }

    tallymarker_hextobin(argv[1], uid_size, strlen(argv[1]));
    card_t *current = card_list_head;

    //Check if card already on list
    if (memcmp(current->uid, uid_size, sizeof(uid_size)) == 0) {
        uart0_puts_p(PSTR("Card not added: UID already in list \r\n"));
        return;
    }

    while (current->next != NULL) {
        current = current->next;

        if (memcmp(current->uid, uid_size, sizeof(uid_size)) == 0) {
            uart0_puts_p(PSTR("Card not added: UID already in list \r\n"));
            return;
        }
    }

    card_t *new_card = NULL;
    new_card = (card_t *)malloc(sizeof(card_t)); //save memory space for card

    //check that malloc has value
    if (new_card == NULL) {
        uart0_puts_p(PSTR("Memory operation failed!\r\n"));
        free(new_card);
        return;
    }

    new_card->uid = malloc(sizeof(uid_size));

    //check that malloc has value
    if (new_card->uid == NULL) {
        uart0_puts_p(PSTR("Memory operation failed!\r\n"));
        free(new_card);
        return;
    }

    memcpy(new_card->uid, uid_size, sizeof(uid_size));
    new_card->size = strlen(argv[1]) / 2;
    //save memory space for holder name
    new_card->holder = malloc((sizeof(char) * strlen(argv[2])) + 1);

    //check that malloc has value
    if (new_card->holder == NULL) {
        uart0_puts_p(PSTR("Memory operation failed!\r\n"));
        free(new_card->uid);
        free(new_card);
        return;
    }

    strcpy(new_card->holder, argv[2]);
    new_card->next = NULL;

    //add card to list
    if (card_list_head == NULL) {
        card_list_head = new_card;
    } else {
        current->next = new_card;
    }

    //print card info
    uart0_puts_p(PSTR("Added card. UID: "));
    print_bytes_for_human(new_card->uid, new_card->size);
    uart0_puts_p(PSTR(" (size: "));
    sprintf_P(print_buf, PSTR("%i bytes)"), new_card->size);
    uart0_puts(print_buf);
    uart0_puts_p(PSTR(" Holder name: "));
    uart0_puts(card_list_head->holder);
    uart0_puts_p(PSTR("\r\n"));
}


void cli_rfid_print(const char *const *argv)
{
    (void) argv;
    card_t *current = card_list_head;
    int list_counter = 1;
    char buffer[256] = {0x00};
    uart0_puts_p(PSTR("Access cards in list: \r\n"));

    //Check if there are cards in list
    if (current == NULL) {
        uart0_puts_p(PSTR("Card list is empty! \r\n"));
    }

    while (current != NULL) {
        sprintf_P(buffer, PSTR("%i. "), list_counter);
        uart0_puts(buffer);
        uart0_puts_p(PSTR("UID["));
        sprintf_P(buffer, PSTR("%i]: "), current->size);
        uart0_puts(buffer);
        print_bytes_for_human(current->uid, current->size);
        uart0_puts_p(PSTR(" Holder name: "));
        uart0_puts(current->holder);
        uart0_puts_p(PSTR("\r\n"));
        current = current->next;
        list_counter++;
    }
}


void cli_rfid_rm(const char *const *argv)
{
    uint8_t uid_size[strlen(argv[1]) / 2];
    tallymarker_hextobin(argv[1], uid_size, strlen(argv[1]));
    card_t *current = card_list_head;
    card_t *prev = NULL;

    //Check if there are cards in list
    if (current == NULL) {
        uart0_puts_p(PSTR("List is empty!\r\n"));
    }

    //Find card
    while (current != NULL) {
        if (memcmp(uid_size, current->uid, current->size) == 0) {
            if (prev == NULL) {
                card_list_head = current->next;
            } else {
                prev->next = current->next;
            }

            uart0_puts_p(PSTR("Removed card UID: "));
            print_bytes_for_human(current->uid, current->size);
            uart0_puts_p(PSTR(" Holder name: "));
            uart0_puts(current->holder);
            uart0_puts_p(PSTR("\r\n"));
            free(current->holder);
            free(current->uid);
            free(current);
            free(prev);
            return;
        }

        prev = current;
        current = current->next;
    }

    uart0_puts_p(PSTR("No card with this UID in list \r\n"));
}


void door_and_disp_handler(void)
{
    Uid uid;
    Uid *uid_ptr = &uid;
    char lcd_buf[16] = {0x00};

    if (PICC_IsNewCardPresent()) {
        PICC_ReadCardSerial(uid_ptr);
        card_t *current = card_list_head;

        //Door is closed if there are no cards in list
        if (current == NULL) {
            door_state = door_closed;
            display_state = display_access_denied;
        }

        while (current != NULL) {
            //check if card is in the list
            if (memcmp(uid.uidByte, current->uid, uid.size) == 0) {
                door_state = door_opening;

                //if previous card did not have access, display name again
                if (match == false) {
                    display_state = display_name;
                } else {
                    display_state = display_no_update;
                }

                match = true;
                card_read_time = time(NULL);

                if (strcmp(display_name_str, current->holder) != 0) {
                    display_state = display_name;
                    display_name_str = current->holder;
                    break;
                } else {
                    return;
                }
            } else {
                match = false;
            }

            current = current->next;
        }

        if (match == false) {
            door_state = door_closing;
            display_state = display_access_denied;
        }
    }

    now_time = time(NULL);

    if (((now_time - card_read_time) > DISPLAY_MSG_IN_SEC) && (match == true)) {
        display_name_str = NULL;
        display_state = display_clear;
    }

    switch (door_state) {
    case door_opening:
        // Document door open time
        door_open_time = time(NULL);
        // Unlock door
        door_state = door_open;
        break;

    case door_open:
        time_y2k_cpy = time(NULL);

        if ((time_y2k_cpy - door_open_time) > DOOR_OPEN_IN_SEC) {
            door_state = door_closing;
        }

        break;

    case door_closing:
        // Lock door
        door_state = door_closed;
        break;

    case door_closed:
        PORTA &= ~_BV(LED_BLU);
        break; // No need to do anything
    }

    switch (display_state) {
    case display_name:

        //turn led on if previous card was not in the list
        if (prev_displ_state != display_name) {
            PORTA ^= _BV(LED_BLU);
        }

        //if name on display doesn't change, then don't show it again
        if ((strcmp(prev_display_name, display_name_str) == 0) &&
                (prev_displ_state == display_name)) {
            display_state = display_clear;
            prev_displ_state = display_name;
            break;
        }

        lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
        lcd_goto(LCD_ROW_2_START);

        if (display_name_str != NULL) {
            strncpy(lcd_buf, display_name_str, LCD_VISIBLE_COLS);
            lcd_puts(lcd_buf);
            prev_display_name = display_name_str;
        } else {
            lcd_puts_P(PSTR("Name read error"));
        }

        msg_display_time = time(NULL);
        display_state = display_clear;
        prev_displ_state = display_name;
        break;

    case display_access_denied:

        //if previous message was the same, then there is no need to update it
        if (prev_displ_state == display_access_denied) {
            display_state = display_clear;
            break;
        }

        lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
        lcd_goto(LCD_ROW_2_START);
        lcd_puts_P(PSTR("Access denied!"));
        msg_display_time = time(NULL);
        display_state = display_clear;
        prev_displ_state = display_access_denied;
        break;

    case display_clear:
        time_y2k_cpy = time(NULL);

        if ((time_y2k_cpy - msg_display_time) > DISPLAY_MSG_IN_SEC) {
            lcd_clr(LCD_ROW_2_START, LCD_VISIBLE_COLS);
            display_state = display_no_update;
            prev_displ_state = display_clear;
        }

        break;

    case display_no_update:
        break;
    }
}