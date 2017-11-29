#include <stdio.h>
#include <avr/pgmspace.h>
#include "print_helper.h"


int print_ascii_tbl(FILE *stream)
{
    for (char c = ' '; c <= '~'; c++) {
        if (!fprintf(stream, "%c ", c)) {
            return 0;
        }
    }

    return fprintf(stream, "\n");
}


int print_for_human(FILE *stream, const unsigned char *array, const size_t len)
{
    if (len > 1) {
        for (unsigned int i = 0; i <= len; i++) {
            unsigned char c = array[i];

            if (c >= ' ' && c <= '~') {
                if (!fprintf(stream, "%c", c)) {
                    return 0;
                }
            } else {
                if (!fprintf(stream, "\"0x%02X\"", c)) {
                    return 0;
                }
            }
        }
    }

    return fprintf(stream, "\n");
}