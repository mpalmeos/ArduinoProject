#include <avr/pgmspace.h>
#ifndef HMI_MSG_H
#define HMI_MSG_H

#define VER_FW "Version: " FW_VERSION " built on: " __DATE__ " " __TIME__ "\r\n"
#define VER_LIBC "avr-libc version: " __AVR_LIBC_VERSION_STRING__ " avr-gcc version: " __VERSION__ "\r\n"
#define MY_NAME "Maie Palmeos"

extern PGM_P const numbers[];

#endif /*HMI_MSG_H */