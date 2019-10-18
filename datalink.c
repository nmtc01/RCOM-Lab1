/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Datalink main file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include "datalink.h"

//Global variables
volatile sig_atomic_t received_ua = 0;
volatile sig_atomic_t received_disc = 0;
volatile sig_atomic_t n_timeouts = 0;
volatile sig_atomic_t break_read_loop = 0;
enum state receiving_ua_state;
enum state receiving_set_state;
enum state receiving_disc_state;

int open_port(int port){
    int fd;
    char serial[11] = "/dev/ttyS0";

    if (port)
        serial[9] = "1";

    fd = open(serial, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (fd < 0)
        return -1;

	message("Opened serial port.");

    return fd;
}

void set_flags(struct termios *oldtio_ptr, struct termios *newtio_ptr, int fd){
    if (tcgetattr(fd, oldtio_ptr) == -1) {
        perror("tcgetattr");
        exit(-1);
    }

    bzero(newtio_ptr, sizeof(struct termios));
    newtio_ptr->c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio_ptr->c_iflag = IGNPAR;
    newtio_ptr->c_oflag = 0;
    newtio_ptr->c_lflag = 0;
    newtio_ptr->c_cc[VTIME] = 0;
    newtio_ptr->c_cc[VMIN] = 1;

    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, newtio_ptr) == -1) {
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

    if (status == TRANSMITTER) {
        //Transmitter

        while(n_timeouts < MAX_TIMEOUTS){
            if (!received_ua) {
                //Write set
                message("Writting set");
                int res_set = write_set(fd);
                if (res_set < 0)
                    return -1;
                alarm(3);

                //Read ua
                message("Reading ua");
                read_ua(fd);
                alarm(0);
            }
            else break;
        }

        //Stop execution if could not stablish connection after MAX_TIMEOUTS
        if (!received_ua)
            return -1;
        
        //Reset number of timeouts and flags
        n_timeouts = 0;
        received_ua = 0;
        break_read_loop = 0;
    }
    else {
        //Receiver

        //Read set
        message("Reading set");
        read_set(fd);

        //Write ua
        message("Writting ua");
        int res_ua = write_ua(fd);

    }
    
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

void read_ua(int fd) {
    char ua[STR_SIZE];
    int res;
    int n_bytes = 0;
    char read_char[1];
    read_char[0] = '\0';

    receiving_ua_state = START;
    break_read_loop = 0;
    
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
                if (read_char[0] == A_CMD) {
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
                if (read_char[0] == A_CMD^C_UA) {
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

int write_ua(int fd) {

  //Create trama UA
  unsigned char ua[5];
  ua[0] = FLAG;
  ua[1] = A_CMD;
  ua[2] = C_UA;
  ua[3] = A_CMD ^ C_UA;
  ua[4] = FLAG;

  // WRITE
  int res = write(fd, ua, 5*sizeof(char));
  printf("%x%x%x%x%x - %d bytes written\n", ua[0],ua[1],ua[2],ua[3],ua[4], res);

  return res;
}