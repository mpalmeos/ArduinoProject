#include <avr/pgmspace.h>
#include "hmi_msg.h"

static const char number_0[] PROGMEM = "Zero";
static const char number_1[] PROGMEM = "One";
static const char number_2[] PROGMEM = "Two";
static const char number_3[] PROGMEM = "Three";
static const char number_4[] PROGMEM = "Four";
static const char number_5[] PROGMEM = "Five";
static const char number_6[] PROGMEM = "Six";
static const char number_7[] PROGMEM = "Seven";
static const char number_8[] PROGMEM = "Eight";
static const char number_9[] PROGMEM = "Nine";

PGM_P const numbers[] PROGMEM = {number_0, number_1, number_2, number_3, number_4, number_5, number_6,
                                 number_7, number_8, number_9
                                };