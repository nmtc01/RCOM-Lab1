/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Datalink main file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include "datalink.h"

//Global variables
struct linkLayer datalink;
volatile sig_atomic_t received_ua = 0;
volatile sig_atomic_t received_disc = 0;
volatile sig_atomic_t n_timeouts = 0;
volatile sig_atomic_t break_read_loop = 0;
struct termios oldtio;
struct termios newtio;
enum state receiving_ua_state;
enum state receiving_set_state;
enum state receiving_disc_state;

void message(char* message){
    printf("!--%s\n", message);
}

int open_port(int port){
    int fd;

    if (port)
        strcpy(datalink.port, "/dev/ttyS1");
    else strcpy(datalink.port, "/dev/ttyS0");

    fd = open(datalink.port, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (fd < 0)
        return -1;

	message("Opened serial port.");

    //Set flags
    set_flags(fd);

    return fd;
}

void set_flags(int fd){
    if (tcgetattr(fd, &oldtio) == -1) {
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(struct termios));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 1;

    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    message("Terminal flags set.");
}

void timeout_handler(){
    if (receiving_ua_state != FINISH) {
        received_ua = 0;
        receiving_ua_state = START;
    }
    else if (receiving_disc_state != FINISH) {
        received_disc = 0;
        receiving_disc_state = START;
    }

    break_read_loop = 1;
    n_timeouts++;

    message("Timed out.");
}

int sendStablishTramas(int fd, int status){
    //Install timeout handler
    (void) signal(SIGALRM, timeout_handler);
    datalink.numTransmissions = 3;
    datalink.timeout = 1;

    if (status == TRANSMITTER) {
        //Transmitter

        while(n_timeouts < datalink.numTransmissions){
            if (!received_ua) {
                //Write set
                message("Writting set");
                int res_set = write_set(fd);
                if (res_set < 0)
                    return -1;
                alarm(datalink.timeout);

                //Read ua
                message("Reading ua");
                read_ua(fd, status);
                alarm(0);
            }
            else break;
        }

        //Stop execution if could not stablish connection after MAX_TIMEOUTS
        if (!received_ua)
            return -1;
        
    }
    else {
        //Receiver

        //Read set
        message("Reading set");
        read_set(fd);

        //Write ua
        message("Writting ua");
        int res_ua = write_ua(fd, status);

    }

    //Reset number of timeouts and flags
        n_timeouts = 0;
        received_ua = 0;
        break_read_loop = 0;
    
    return 0;
}

int write_set(int fd) {
    //Create trama SET
    unsigned char set[5];
    set[0] = FLAG;
    set[1] = A_CMD;
    set[2] = C_SET;
    set[3] = A_CMD ^ C_SET;
    set[4] = FLAG;

    int res = write(fd, set, 5*sizeof(char));
    printf("%x%x%x%x%x - %d bytes written\n", set[0], set[1], set[2], set[3], set[4], res);

    return res;
}

void read_ua(int fd, int status) {
    char ua[STR_SIZE];
    int res;
    int n_bytes = 0;
    char read_char[1];
    read_char[0] = '\0';

    receiving_ua_state = START;
    break_read_loop = 0;

    u_int8_t A;
    if (status == TRANSMITTER)
        A = A_CMD;
    else A = A_ANS;

    while (!break_read_loop) {
        res = read(fd, read_char, sizeof(char));	
        ua[n_bytes] = read_char[0];
        
        switch (receiving_ua_state) {
            case START:
            {
                if (read_char[0] == FLAG) {
                    receiving_ua_state = FLAG_RCV;
                    n_bytes++;
                }
                break;
            }
            case FLAG_RCV:
            {
                if (read_char[0] == A) {
                    receiving_ua_state = A_RCV;
                    n_bytes++;
                }
                else if (read_char[0] == FLAG) {
                    n_bytes = 1;
                    break;
                }
                else {
                    receiving_ua_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case A_RCV:
            {
                if (read_char[0] == C_UA) {
                    receiving_ua_state = C_RCV;
                    n_bytes++;
                }
                else if (read_char[0] == FLAG) { 
                    receiving_ua_state = FLAG_RCV;
                    n_bytes = 1;
                }
                else {
                    receiving_ua_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case C_RCV:
            {
                if (read_char[0] == A^C_UA) {
                    receiving_ua_state = BCC_OK;
                    n_bytes++;
                }
                else if (read_char[0] == FLAG) {
                    receiving_ua_state = FLAG_RCV;
                    n_bytes = 1;
                }
                else {
                    receiving_ua_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case BCC_OK:
            {
                if (read_char[0] == FLAG) {
                    receiving_ua_state = FINISH;
                    n_bytes++;
                }
                else {
                    receiving_ua_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case FINISH:
            {
                break_read_loop = 1;
                received_ua = 1;
                break;
            }
        }
    }

    printf("%x%x%x%x%x - %d bytes read\n", ua[0], ua[1], ua[2], ua[3], ua[4], n_bytes);
}

void read_set(int fd) {
    char set[STR_SIZE];
    char read_char[2];
    int n_bytes = 0;
    int res;
    int received_set = 0;
    receiving_set_state = START;

    // READ
    while (!received_set) {
        res = read(fd, read_char, sizeof(char));               
        set[n_bytes] = read_char[0];     
        
        switch (receiving_set_state) {
        case START:
        {
            if (read_char[0] == FLAG) {
            receiving_set_state = FLAG_RCV;
            n_bytes++;
            }
            break;
        }
        case FLAG_RCV:
        {
            if (read_char[0] == A_CMD) {
            receiving_set_state = A_RCV;
            n_bytes++;
            }
            else if (read_char[0] == FLAG) {
            n_bytes = 1;
            break;
            }
            else {
            receiving_set_state = START;
            n_bytes = 0;
            }
            break;
        }
        case A_RCV:
        {
            if (read_char[0] == C_SET) {
            receiving_set_state = C_RCV;
            n_bytes++;
            }
            else if (read_char[0] == FLAG) {
            receiving_set_state = FLAG_RCV;
            n_bytes = 1;
            }
            else {
            receiving_set_state = START;
            n_bytes = 0;
            }
            break;
        }
        case C_RCV:
        {
            if (read_char[0] == A_CMD^C_SET) {
            receiving_set_state = BCC_OK;
            n_bytes++;
            }
            else if (read_char[0] == FLAG) {
            receiving_set_state = FLAG_RCV;
            n_bytes = 1;
            }
            else {
            receiving_set_state = START;
            n_bytes = 0;
            }
            break;
        }
        case BCC_OK:
        {
            if (read_char[0] == FLAG) {
            receiving_set_state = FINISH;
            n_bytes++;
            }
            else {
            receiving_set_state = START;
            n_bytes = 0;
            }
            break;
        }
        case FINISH:
        {
            received_set = 1;
            break;
        }
        }
    }
    printf("%x%x%x%x%x - %d bytes read\n", set[0],set[1],set[2],set[3],set[4], n_bytes);
    
}

int write_ua(int fd, int status) {
    
    //Create trama UA
    unsigned char ua[5];
    ua[0] = FLAG;
    if (status == TRANSMITTER)
        ua[1] = A_ANS;
    else ua[1] = A_CMD;
    ua[2] = C_UA;
    if (status == TRANSMITTER)
        ua[3] = A_ANS ^ C_UA;
    else ua[3] = A_CMD ^ C_UA;
    ua[4] = FLAG;

    // WRITE
    int res = write(fd, ua, 5*sizeof(char));
    printf("%x%x%x%x%x - %d bytes written\n", ua[0],ua[1],ua[2],ua[3],ua[4], res);

    return res;
}

int sendDiscTramas(int fd, int status) {
    if (status == TRANSMITTER) {
        //Transmitter

        while(n_timeouts < datalink.numTransmissions){
            if (!received_disc) {
                //Write disc
                message("Writting disc");
                int res_disc = write_disc(fd, status);
                if (res_disc < 0)
                    return -1;
                alarm(datalink.timeout);

                //Read disc
                message("Reading disc");
                read_disc(fd, status);
                alarm(0);
            }
        else break;
        }

        //Stop execution if could not stablish connection after MAX_TIMEOUTS
        if (!received_disc)
            return -1;

        //Write ua
        message("Writting ua");
        int res_ua = write_ua(fd, status);
        if (res_ua < 0)
            return -1;

    }
    else {
        //Receiver

        //Read disc
        message("Reading disc");
        read_disc(fd, status); 
        
        while(n_timeouts < datalink.numTransmissions) {
            if (!received_ua) {
                //Write disc
                message("Writting disc");
                int res_disc = write_disc(fd, status); 
                if (res_disc < 0)
                    return -1;
                alarm(datalink.timeout);
                
                //Read ua
                message("Reading ua");
                read_ua(fd, status);
                alarm(0);
            }
            else break;
        }
    }

    return 0;
}

int write_disc(int fd, int status) {
    //Create trama DISC
    unsigned char disc[5];
    disc[0] = FLAG;
    if (status == TRANSMITTER)
        disc[1] = A_CMD;
    else disc[1] = A_ANS;
    disc[2] = C_DISC;
    if (status == TRANSMITTER)
        disc[3] = A_CMD ^ C_DISC;
    else disc[3] = A_ANS ^ C_DISC;
    disc[4] = FLAG;

    int res = write(fd, disc, 5*sizeof(char));
    printf("%x%x%x%x%x - %d bytes written\n", disc[0], disc[1], disc[2], disc[3], disc[4], res);

    return res;
}

void read_disc(int fd, int status) {
    char disc[STR_SIZE];
    int res;
    int n_bytes = 0;
    char read_char[1];
    read_char[0] = '\0';
    u_int8_t A;
    if (status == TRANSMITTER)
        A = A_ANS;
    else A = A_CMD;

    receiving_disc_state = START;
    break_read_loop = 0;

    while (!break_read_loop) {
        res = read(fd, read_char, sizeof(char));	
        disc[n_bytes] = read_char[0];
        
        switch (receiving_disc_state) {
            case START:
            {
                if (read_char[0] == FLAG) {
                    receiving_disc_state = FLAG_RCV;
                    n_bytes++;
                }
                break;
            }
            case FLAG_RCV:
            {
                if (read_char[0] == A) {
                    receiving_disc_state = A_RCV;
                    n_bytes++;
                }
                else if (read_char[0] == FLAG) {
                    n_bytes = 1;
                    break;
                }
                else {
                    receiving_disc_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case A_RCV:
            {
                if (read_char[0] == C_DISC) {
                    receiving_disc_state = C_RCV;
                    n_bytes++;
                }
                else if (read_char[0] == FLAG) { 
                    receiving_disc_state = FLAG_RCV;
                    n_bytes = 1;
                }
                else {
                    receiving_disc_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case C_RCV:
            {
                if (read_char[0] == A^C_DISC) {
                    receiving_disc_state = BCC_OK;
                    n_bytes++;
                }
                else if (read_char[0] == FLAG) {
                    receiving_disc_state = FLAG_RCV;
                    n_bytes = 1;
                }
                else {
                    receiving_disc_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case BCC_OK:
            {
                if (read_char[0] == FLAG) {
                    receiving_disc_state = FINISH;
                    n_bytes++;
                }
                else {
                    receiving_disc_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case FINISH:
            {
                break_read_loop = 1;
                received_disc = 1;
                break;
            }
            }
    }

    printf("%x%x%x%x%x - %d bytes read\n", disc[0], disc[1], disc[2], disc[3], disc[4], n_bytes);
}

void cleanup(int fd){
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    close(fd);

    message("Cleaned up terminal.");
}