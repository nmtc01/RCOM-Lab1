/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Datalink header file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include <fcntl.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <sys/times.h>
#include <time.h>

//Efficiency
#define ERROR_PROB 5
#define BAUD_VALUE 38400.0

#define BAUDRATE        B38400
#define MAX_FRAME_SIZE  512
#define MAX_DATA_SIZE   (MAX_FRAME_SIZE - 6)
#define TRANSMITTER     12
#define RECEIVER        21
#define COM1            0
#define COM2            1
#define COM3            2
#define COM4            3
#define COM5            4

// FLAGS
#define FLAG            0x7E
#define A_CMD           0x03
#define A_ANS           0x01
#define C_SET           0x03
#define C_UA            0x07
#define C_DISC          0x0B
#define C_0             0x00
#define C_1             0x40
#define C_RR0           0x05
#define C_RR1           0x85
#define C_REJ0          0x01
#define C_REJ1          0x81
#define ESCAPE          0x7D
#define STUF            0x20
#define REJECT_DATA     1

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

//Open
int llopen(int port, int status);

//Write
int llwrite(int fd, unsigned char* packet, int length);

//Read
int llread(int fd, unsigned char* buffer);

//Close
int llclose(int fd, int status);

void message(char *message);
void message_packet(int i);
void minor_message(char *message);
int open_port(int port);
void set_flags(int fd);
void cleanup(int fd);
void timeout_handler();

int sendStablishTramas(int fd, int status);

int write_set(int fd);
void read_ua(int fd, int status);
void read_set(int fd);
int write_ua(int fd, int status);
int write_disc(int fd, int status);
void read_disc(int fd, int status);
int write_i(int fd, char *buffer, int length);
int read_i(int fd, char *buffer, int *reject);
int write_rr(int fd);
int read_rr(int fd);
int write_rej(int fd);

// Eficiency
void generate_errors(unsigned char* buffer);
