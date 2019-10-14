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
#define A_CMD 0x03
#define A_ANS 0x01
#define C_UA 0x07
#define C_SET 0x03

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
int read_msg(int *fd_ptr, unsigned char *buf);
int write_msg(int *fd_ptr, int n_bytes);
void cleanup(struct termios *oldtio_ptr, int *fd_ptr);