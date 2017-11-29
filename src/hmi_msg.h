#include <avr/pgmspace.h>
#ifndef HMI_MSG_H
#define HMI_MSG_H

#define CUR_VERSION "Version: %S built on: %S %S\n"
#define AVR_VERSION "avr-libc version: %S avr-gcc version: %S\n"
#define MY_NAME "Maie Palmeos"
#define GET_NUMBER_MSG "\nPlease enter a number >"
#define GIVE_NUMBER_MSG "\nYou entered number "
#define NOT_NUMBER_MSG "\nPlease enter a number between 0 and 9"
const char numbers[10][6] PROGMEM = {"Zero", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine"};

#endif /*HMI_MSG_H */