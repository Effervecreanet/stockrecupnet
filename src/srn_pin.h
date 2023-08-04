#define PIN_SIZE 6
#define SRN_DB "db/pin_file.txt"

int gen_pin(unsigned char pin[PIN_SIZE + 1], char filename[254]);
int pin_exist(unsigned char pin[PIN_SIZE + 1], char filename[254]);
int rec_pin(unsigned char pin[PIN_SIZE + 1], char *filename);
int rem_pin(long fposmatch, unsigned short linelen);
void createdb(void);
