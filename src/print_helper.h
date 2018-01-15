#ifndef PRINT_HELPER_H
#define PRINT_HELPER_H

void print_ascii_tbl ();
void print_for_human (const unsigned char *array, const size_t len);
void print_bytes_for_human(const uint8_t bite);
uint8_t tallymarker_hextobin(const char * str, uint8_t * bytes, size_t blen);

#endif /*PRINT_HELPER_H */