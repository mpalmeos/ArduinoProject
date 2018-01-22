#ifndef RFID_H
#define RFID_H

void cli_rfid_read(const char *const *argv);
void cli_rfid_print(const char *const *argv);
void cli_rfid_add(const char *const *argv);
void cli_rfid_rm(const char *const *argv);
void door_and_disp_handler(void);

#endif /*RFID_H */