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
        //Read file to send
        char *file;
        int numbytes = readFile(file);
        if (numbytes < 0) {
            perror("readFile");
            return -1;
        }

        char msg_to_send[4] = "seu";
        message("Started llwrite");
        int n_chars_written = llwrite(application.fd_port, msg_to_send, 4*sizeof(char));
        if (n_chars_written < 0) {
            perror("llwrite");
            return -1;
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

int readFile(char *msg) {
    FILE *file;
    int numbytes;

    //open an existing file for reading
    file = fopen(FILE_TO_SEND, "r");
    
    //quit if the file does not exist
    if (file == NULL) 
        return -1;
    
    //get the number of bytes
    if (fseek(file, 0L, SEEK_END))
        return -1;
    numbytes = ftell(file);

    //reset the file position indicator
    fseek(file, 0L, SEEK_SET);

    //allocate memory
    msg = (char*)calloc(numbytes, sizeof(char));
    if (msg == NULL)
        return -1;

    //copy all the text into msg
    fread(msg, sizeof(char), numbytes, file);
    
    //close file
    if (fclose(file))
        return -1;

    return numbytes;
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

/*
    TODO -> Finish sendITramas and receiveITramas:
    fazer primeira experiência de mandar apenas "ola" dentro dos dados da trama I
    depois:
    timeout nas tramas I
    RR e ACK
    colocar a ler o ficheiro 
    colocar em loop o sendITramas
    colocar a mandar as tramas como é suposto
    colocar a ler as tramas com máquina de estados
    fazer o caso em que é recebido o octeto de escape
    fazer o destuffing
    colocar a construir o ficheiro
*/