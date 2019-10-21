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
#define C_0 0x00
#define C_1 0x40
#define C_RR0 0x05
#define C_RR1 0x85
#define ESCAPE 0x7D
#define STUF 0x20

enum state {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    FINISH
};

enum dataState {
    START_I,
    FLAG_RCV_I,
    A_RCV_I,
    C_RCV_I,
    BCC1_OK_I,
    DATA_RCV_I,
    FLAG2_RCV_I,
    FINISH_I,
    ESCAPE_RCV_I
};

//Datalinker struct
struct linkLayer {
    char port[11];
    int baudRate;
    unsigned int sequenceNumber;
    unsigned int timeout;
    unsigned int numTransmissions;
};

void message(char* message);
int open_port(int port);
void set_flags(int fd);
void cleanup(int fd);
void timeout_handler();

int sendStablishTramas(int fd, int status);
int sendDiscTramas(int fd, int status);
int sendITramas(int fd, char *buffer, int length);
int receiveITramas(int fd, char *buffer);

int write_set(int fd);
void read_ua(int fd, int status);
void read_set(int fd);
int write_ua(int fd, int status);
int write_disc(int fd, int status);
void read_disc(int fd, int status);
int write_i(int fd, char *buffer, int length);
int read_i(int fd, char *buffer);
int write_rr(int fd);
void read_rr(int fd);