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


void setup(int argc, char** argv);
