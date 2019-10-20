/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Application header file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */  

#include "datalink.h"

#define FILE_TO_SEND "images/pinguim.gif"
#define CNTRL_START  2
#define CNTRL_END    3

//Application struct
struct appLayer {
    int fd_port;
    int status;
};

//Open
void setup(int argc, char **argv);
int llopen(int port, int status);

//Write
int readFile(char *msg);
int llwrite(int fd, char *buffer, int length);

//Read
int llread(int fd, char *buffer);

//Close
int llclose(int fd, int status);