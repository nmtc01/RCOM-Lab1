/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Emitter header file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1
#define FALSE 0
#define TRUE 1

// FLAGS
#define FLAG 0x7E
#define A_CMD 0x03
#define A_ANS 0x01
#define C_SET 0x03
#define C_UA 0x07

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

void timeout_handler();
void message(char* message){printf("!--%s\n", message);}

void setup(int argc, char **argv);
void open_port(char **argv, int *fd_ptr);
void set_flags(struct termios *oldtio_ptr, struct termios *newtio_ptr, int *fd_ptr);
int write_set(int *fd_ptr);
void read_ua(int *fd_ptr, unsigned char *buf);
void cleanup(struct termios *oldtio_ptr, int *fd_ptr);
