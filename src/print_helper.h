#ifndef PRINT_HELPER_H
#define PRINT_HELPER_H

int print_ascii_tbl (FILE *stream);
int print_for_human (FILE *stream, const unsigned char *array,
                     const size_t len);

#endif /*PRINT_HELPER_H */