#ifndef RFID_H
#define RFID_H

typedef struct card {
    uint8_t *uid;
    uint8_t size;
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

void cli_rfid_read(const char *const *argv);
void cli_rfid_print(const char *const *argv);
void cli_rfid_add(const char *const *argv);
void cli_rfid_rm(const char *const *argv);
card_t* rfid_card_finder(const card_t *n_card);

#endif /*RFID_H */