/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Application main file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include "application.h"

int main(int argc, char **argv) {
    
    //Validate arguments
    setup(argc, argv);

    //Application struct
    struct appLayer application;
    if (argv[1] == "transmitter")
        application.status = TRANSMITTER;
    else application.status = RECEIVER;

    //Port
    int port;
    if ((strcmp("/dev/ttyS0", argv[2]) == 0))
        port = COM1;
    else port = COM2;

    //Stablish communication
    application.fd_port = llopen(port, application.status);
    if (application.fd_port < 0) {
        perror("llopen");
        return -1;
    }

    return 0;       
}

void message(char* message){
    printf("!--%s\n", message);
}

void setup(int argc, char **argv){
	if ((argc != 3) || ((strcmp("/dev/ttyS0", argv[2]) != 0) && (strcmp("/dev/ttyS1", argv[2]) != 0))) {
        printf("Usage:\tnserial transmitter|receiver SerialPort\n\tex: nserial /dev/ttyS0\n");
        exit(1);
    }

	message("Checked arguments");
}

int llopen(int port, int status) {

    //Open serial port
    int fd = open_port(port);
    //Check errors
    if (fd < 0)
        return -1;

    //Set flags
    struct termios oldtio;
    struct termios newtio;
    set_flags(&oldtio, &newtio, fd);

    //Tramas set and ua
    int res = sendStablishTramas(fd, status);
    //Check errors
    if (res < 0)
        return -1;

    return fd;
}