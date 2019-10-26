/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Application header file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include "packet.h"

//Application struct
struct appLayer {
    int fd_port;
    int status;
};

//Open
void setup(int argc, char** argv);
int llopen(int port, int status);

//Write
int llwrite(int fd, char* packet, int length);

//Read
int llread(int fd, char* buffer);

//Close
int llclose(int fd, int status);