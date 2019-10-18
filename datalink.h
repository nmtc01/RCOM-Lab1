/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Datalink header file
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

#define BAUDRATE        B38400
#define MAX_TIMEOUTS    3
#define STR_SIZE        255
#define TRANSMITTER     12
#define RECEIVER        21
#define COM1            0
#define COM2            1

// FLAGS
#define FLAG 0x7E
#define A_CMD 0x03
#define A_ANS 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B

enum state {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    FINISH
};

int open_port(int port);
void set_flags(struct termios *oldtio_ptr, struct termios *newtio_ptr, int fd);
void timeout_handler();
int sendStablishTramas(int fd, int status);

int write_set(int fd);
void read_ua(int fd);
void read_set(int fd);
int write_ua(int fd);