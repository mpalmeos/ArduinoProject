#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <avr/pgmspace.h>
#include "../lib/hd44780_111/hd44780.h"
#include "../lib/andygock_avr-uart/uart.h"
#include "../lib/helius_microrl/microrl.h"
#include "../lib/andy_brown_memdebug/memdebug.h"
#include "../lib/matejx_avr_lib/mfrc522.h"
#include "hmi_msg.h"
#include "print_helper.h"
#include "cli_microrl.h"
#include "rfid.h"


static card_t *card_list_head = NULL;


void cli_rfid_read(const char *const *argv)
{
    (void) argv;
    Uid uid;
    Uid *uid_ptr = &uid;

    if (PICC_IsNewCardPresent()) {
        uart0_puts_p(PSTR("Card selected!\r\n"));
        PICC_ReadCardSerial(uid_ptr);
        uart0_puts_p(PSTR("Card type: "));
        uart0_puts(PICC_GetTypeName(PICC_GetType(uid.sak)));
        uart0_puts_p(PSTR("\r\n"));
        uart0_puts_p(PSTR("Card UID: "));

        for (byte i = 0; i < uid.size; i++) {
            print_bytes_for_human(uid.uidByte[i]);
        }

        uart0_puts_p(PSTR(" (size "));
        print_bytes_for_human(uid.size);
        uart0_puts_p(PSTR(" bytes)"));
        uart0_puts_p(PSTR("\r\n"));
    } else {
        uart0_puts_p(PSTR("Unable to select card!\r\n"));
    }
}


card_t* rfid_card_finder(const card_t *n_card)
{
    if (card_list_head != NULL) {
        card_t *current = card_list_head;

        while (current != NULL) {
            if (current->size == n_card->size) {
                for (size_t i = 0; i < current->size; i++) {
                    if (current->uid[i] == n_card->uid[i]) {
                        return current;
                    }
                }
            }

            current = current->next;
        }
    }

    return NULL;
}


void cli_rfid_add(const char *const *argv)
{
    //Save card info
    card_t *new_card;
    new_card = (card_t *)malloc(sizeof(card_t)); //save memory space for card
    new_card->size = strlen(argv[1]) /
                     2; //get card size (in HEX each 2 characters are 1 byte)
    new_card->uid = (uint8_t *)malloc(new_card->size * sizeof(
                                          uint8_t)); //save memory space for UID
    new_card->holder = malloc(sizeof(char) * strlen(
                                  argv[2])); //save memory space for holder name
    strcpy(new_card->holder, argv[2]); //save card holder name from second argument
    tallymarker_hextobin(argv[1], new_card->uid,
                         new_card->size); //turn hex values to byte array

    //check UID length
    if (new_card->size > 10) {
        uart0_puts_p(PSTR("UID is too long! Max length is 10 bytes.\r\n"));
        free(new_card);
        return;
    }

    //check that malloc has value
    if (new_card == NULL) {
        uart0_puts_p(PSTR("Memory operation failed!\r\n"));
        free(new_card);
        return;
    }

    //check if card already on list
    card_t *compare = rfid_card_finder(new_card);

    if (compare) {
        uart0_puts_p(PSTR("Card not added: UID already in list \r\n"));
        return;
    }

    //add card to list
    new_card->next = card_list_head;
    card_list_head = new_card;
    //print card info
    uart0_puts_p(PSTR("Added card. UID: "));

    for (size_t i = 0; i < card_list_head->size; i++) {
        print_bytes_for_human(card_list_head->uid[i]);
    }

    uart0_puts_p(PSTR(" ("));
    print_bytes_for_human(card_list_head->size);
    uart0_puts_p(PSTR(" bytes) Holder name: "));
    uart0_puts(card_list_head->holder);
    uart0_puts_p(PSTR("\r\n"));
}


void cli_rfid_print(const char *const *argv)
{
    (void) argv;
    card_t *current = card_list_head;
    int list_counter = 1;
    char buffer[10];

    if (current == NULL) {
        uart0_puts_p(PSTR("Card list is empty \r\n"));
    }

    while (current != NULL) {
        sprintf_P(buffer, PSTR("%u. "), list_counter);
        uart0_puts(buffer);
        uart0_puts_p(PSTR("UID["));
        print_bytes_for_human(current->size);
        uart0_puts_p(PSTR("]: "));

        for (size_t i = 0; i < current->size; i++) {
            print_bytes_for_human(current->uid[i]);
        }

        uart0_puts_p(PSTR(" Holder name: "));
        uart0_puts(current->holder);
        uart0_puts_p(PSTR("\r\n"));
        current = current->next;
        list_counter++;
    }
}


void cli_rfid_rm(const char *const *argv)
{
    if (card_list_head == NULL) {
        uart0_puts_p(PSTR("List is empty!\r\n"));
    }

    card_t *current = card_list_head;
    card_t *prev = card_list_head;
    size_t rm_card_size = strlen(argv[1]) / 2; //find card size from input
    uint8_t *rm_card_uid = malloc(rm_card_size * sizeof(
                                      uint8_t)); //save memory space for removable card
    tallymarker_hextobin(argv[1], rm_card_uid, rm_card_size);

    //find card from list
    while (current != NULL) {
        if (current->size == rm_card_size) {
            for (size_t i = 0; i < rm_card_size; i++) {
                if (current->uid[i] == rm_card_uid[i]) {
                    if (prev == NULL) { //In case there is only one item in list
                        card_list_head = current->next;
                        uart0_puts_p(PSTR("Removed card UID: "));

                        for (size_t i = 0; i < rm_card_size; i++) {
                            print_bytes_for_human(rm_card_uid[i]);
                        }

                        uart0_puts_p(PSTR("\r\n"));
                        free(rm_card_uid);
                        free(current->uid);
                        free(current->holder);
                        free(current);
                        return;
                    } else {
                        prev->next = current->next;
                        uart0_puts_p(PSTR("Removed card UID: "));

                        for (size_t i = 0; i < rm_card_size; i++) {
                            print_bytes_for_human(rm_card_uid[i]);
                        }

                        uart0_puts_p(PSTR("\r\n"));
                        free(rm_card_uid);
                        free(current->uid);
                        free(current->holder);
                        free(current);
                        return;
                    }
                }
            }
        }

        prev = current;
        current = current->next;
    }

    uart0_puts_p(PSTR("No card with this UID in list \r\n"));
}