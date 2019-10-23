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
    if (strcmp(argv[1], "transmitter") == 0)
        application.status = TRANSMITTER;
    else application.status = RECEIVER;

    //Port
    int port;
    if ((strcmp("/dev/ttyS0", argv[2]) == 0))
        port = COM1;
    else port = COM2;

    //Stablish communication
    message("Started llopen");
    application.fd_port = llopen(port, application.status);
    if (application.fd_port < 0) {
        perror("llopen");
        return -1;
    }
    
    //Main Communication
    if (application.status == TRANSMITTER) {
        //Transmitter
        //Open file
        int fd_file = open(FILE_TO_SEND, O_RDONLY | O_NONBLOCK);
        if (fd_file < 0) {
            perror("Opening File");
            return -1;
        }

        //Fragments of file
        unsigned char frag[FRAG_SIZE];
        int numbytes;

        //Write information
        message("Started llwrite");
        //Read fragments and send them one by one
        while ((numbytes = read(fd_file, frag, FRAG_SIZE)) != 0) {
            if (numbytes < 0) {
                perror("readFile");
                return -1;
            }
            int n_chars_written = llwrite(application.fd_port, frag, FRAG_SIZE);
            if (n_chars_written < 0) {
                perror("llwrite");
                return -1;
            }
        }
    }
    else {
        //Receive information
        message("Started llread");
        char msg_to_receive[4];
        int n_chars_read = llread(application.fd_port, msg_to_receive);
        if (n_chars_read < 0) {
            perror("llread");
            return -1;
        }
    }

    //Finish communication
    message("Started llclose");
    if (llclose(application.fd_port, application.status) < 0) {
        perror("llclose");
        return -1;
    }

    return 0;       
}

void setup(int argc, char **argv){
	if ((argc != 3) || ((strcmp("/dev/ttyS0", argv[2]) != 0) && (strcmp("/dev/ttyS1", argv[2]) != 0))) {
        printf("Usage:\tnserial transmitter|receiver SerialPort\n\tex: nserial transmitter /dev/ttyS0\n");
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

    //Tramas set and ua
    int res = sendStablishTramas(fd, status);
    //Check errors
    if (res < 0)
        return -1;

    return fd;
}

int llwrite(int fd, char *buffer, int length) {
    int nr_chars = sendITramas(fd, buffer, length);
    if (nr_chars < 0)
        return -1;

    return nr_chars;
} 

int llread(int fd, char *buffer) {
    int nr_chars = receiveITramas(fd, buffer);
    if (nr_chars < 0)
        return -1;

    return nr_chars;
}

int llclose(int fd, int status) {
    int res = sendDiscTramas(fd, status);
    if (res < 0)
        return -1;

    cleanup(fd);

    return 1;
}
