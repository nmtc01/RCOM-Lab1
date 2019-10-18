#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

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

// VALUES
#define STR_SIZE 255
#define MAX_TIMEOUTS 3

enum state {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    FINISH
};

volatile int STOP = FALSE;

void timeout_handler();
void message(char* message);

void setup(int argc, char **argv);
void open_port(char **argv, int *fd_ptr);
void set_flags(struct termios *oldtio_ptr, struct termios *newtio_ptr, int *fd_ptr);
int read_set(int fd, unsigned char *buf);
int write_ua(int fd, int n_bytes);
int read_disc(int fd, unsigned char *request);
int write_disc(int fd, int n_bytes);
int read_ua(int fd, unsigned char *answer);
void cleanup(struct termios *oldtio_ptr, int fd);

