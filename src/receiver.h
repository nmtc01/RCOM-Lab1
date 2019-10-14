#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include<unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1
#define FALSE 0
#define TRUE 1

// FLAGS
#define FLAG 0x7E
#define A_ANS 0x03
#define A_CMD 0x01
#define C_UA 0x07
#define C_SET 0x03
#define C_DISC 0x0B

// SIZES
#define STR_SIZE 255

enum state {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    FINISH
};

volatile int STOP = FALSE;

void message(char* message);
void setup(int argc, char **argv);
void open_port(char **argv, int *fd_ptr);
void set_flags(struct termios *oldtio_ptr, struct termios *newtio_ptr, int *fd_ptr);
int read_set(int *fd_ptr, unsigned char *buf);
int write_ua(int *fd_ptr, int n_bytes);
int read_disc(int *fd_ptr, unsigned char *request);
int write_disc(int *fd_ptr, int n_bytes);
int read_ua(int *fd_ptr, unsigned char *answer);
void cleanup(struct termios *oldtio_ptr, int *fd_ptr);
