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
volatile sig_atomic_t received_i = 0;
volatile sig_atomic_t n_timeouts = 0;
volatile sig_atomic_t break_read_loop = 0;
volatile sig_atomic_t timed_out = 0;
volatile sig_atomic_t nr_tramaI = 0;
volatile sig_atomic_t control_start = 1;
struct termios oldtio;
struct termios newtio;
enum state receiving_ua_state;
enum state receiving_set_state;
enum state receiving_disc_state;
enum state receiving_rr_state;
enum dataState receiving_data_state;

void message(char *message) {
    printf("!--%s\n", message);
}

int open_port(int port) {
    int fd;

    if (port)
        strcpy(datalink.port, "/dev/ttyS1");
    else
        strcpy(datalink.port, "/dev/ttyS0");

    fd = open(datalink.port, O_RDWR | O_NOCTTY);

    if (fd < 0)
        return fd;

    message("Opened serial port.");

    //Set flags
    set_flags(fd);

    return fd;
}

void set_flags(int fd) {
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

    //Initial sequenceNumber
    datalink.sequenceNumber = 0;

    message("Terminal flags set.");
}

void timeout_handler() {
    if (receiving_ua_state != FINISH) {
        received_ua = 0;
        receiving_ua_state = START;
    } else if (receiving_disc_state != FINISH) {
        received_disc = 0;
        receiving_disc_state = START;
    } else if (receiving_data_state != FINISH_I) {
        received_i = 0;
        receiving_data_state = START_I;
    }

    timed_out = 1;
    break_read_loop = 1;
    n_timeouts++;

    message("Timed out.");
}

int sendStablishTramas(int fd, int status) {
    //Install timeout handler
    (void)signal(SIGALRM, timeout_handler);
    datalink.numTransmissions = 3;
    datalink.timeout = 1;

    if (status == TRANSMITTER) {
        //Transmitter

        while (n_timeouts < datalink.numTransmissions) {
            if (!received_ua) {
                //Write set
                message("Writting set");
                int res_set = write_set(fd);
                if (res_set < 0)
                    return res_set;
                alarm(datalink.timeout);

                //Read ua
                message("Reading ua");
                read_ua(fd, status);
                alarm(0);
            } else
                break;
        }

        //Stop execution if could not stablish connection after MAX_TIMEOUTS
        if (!received_ua)
            return -1;

    } else {
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

    int res = write(fd, set, 5 * sizeof(char));
    printf("%02x%02x%02x%02x%02x - %d bytes written\n", set[0], set[1], set[2], set[3], set[4], res);

    return res;
}

void read_ua(int fd, int status) {
    unsigned char ua[STR_SIZE];
    int res;
    int n_bytes = 0;
    unsigned char read_char[1];
    read_char[0] = '\0';

    receiving_ua_state = START;
    break_read_loop = 0;

    u_int8_t A;
    if (status == TRANSMITTER)
        A = A_CMD;
    else
        A = A_ANS;

    while (!break_read_loop) {
        if (receiving_ua_state != FINISH) {
            res = read(fd, read_char, sizeof(char));
            ua[n_bytes] = read_char[0];
        }

        switch (receiving_ua_state) {
            case START: {
                if (read_char[0] == FLAG) {
                    receiving_ua_state = FLAG_RCV;
                    n_bytes++;
                }
                break;
            }
            case FLAG_RCV: {
                if (read_char[0] == A) {
                    receiving_ua_state = A_RCV;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    n_bytes = 1;
                    break;
                } else {
                    receiving_ua_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case A_RCV: {
                if (read_char[0] == C_UA) {
                    receiving_ua_state = C_RCV;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    receiving_ua_state = FLAG_RCV;
                    n_bytes = 1;
                } else {
                    receiving_ua_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case C_RCV: {
                if (read_char[0] == A ^ C_UA) {
                    receiving_ua_state = BCC_OK;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    receiving_ua_state = FLAG_RCV;
                    n_bytes = 1;
                } else {
                    receiving_ua_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case BCC_OK: {
                if (read_char[0] == FLAG) {
                    receiving_ua_state = FINISH;
                    n_bytes++;
                } else {
                    receiving_ua_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case FINISH: {
                break_read_loop = 1;
                received_ua = 1;
                break;
            }
        }
    }

    printf("%02x%02x%02x%02x%02x - %d bytes read\n", ua[0], ua[1], ua[2], ua[3], ua[4], n_bytes);
}

void read_set(int fd) {
    unsigned char set[STR_SIZE];
    unsigned char read_char[1];
    int n_bytes = 0;
    int res;
    int received_set = 0;
    receiving_set_state = START;

    // READ
    while (!received_set) {
         if (receiving_set_state != FINISH) {
            res = read(fd, read_char, sizeof(char));
            set[n_bytes] = read_char[0];
         }

        switch (receiving_set_state) {
            case START: {
                if (read_char[0] == FLAG) {
                    receiving_set_state = FLAG_RCV;
                    n_bytes++;
                }
                break;
            }
            case FLAG_RCV: {
                if (read_char[0] == A_CMD) {
                    receiving_set_state = A_RCV;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    n_bytes = 1;
                    break;
                } else {
                    receiving_set_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case A_RCV: {
                if (read_char[0] == C_SET) {
                    receiving_set_state = C_RCV;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    receiving_set_state = FLAG_RCV;
                    n_bytes = 1;
                } else {
                    receiving_set_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case C_RCV: {
                if (read_char[0] == A_CMD ^ C_SET) {
                    receiving_set_state = BCC_OK;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    receiving_set_state = FLAG_RCV;
                    n_bytes = 1;
                } else {
                    receiving_set_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case BCC_OK: {
                if (read_char[0] == FLAG) {
                    receiving_set_state = FINISH;
                    n_bytes++;
                } else {
                    receiving_set_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case FINISH: {
                received_set = 1;
                break;
            }
        }
    }
    printf("%02x%02x%02x%02x%02x - %d bytes read\n", set[0], set[1], set[2], set[3], set[4], n_bytes);
}

int write_ua(int fd, int status) {
    //Create trama UA
    unsigned char ua[5];
    ua[0] = FLAG;
    if (status == TRANSMITTER)
        ua[1] = A_ANS;
    else
        ua[1] = A_CMD;
    ua[2] = C_UA;
    if (status == TRANSMITTER)
        ua[3] = A_ANS ^ C_UA;
    else
        ua[3] = A_CMD ^ C_UA;
    ua[4] = FLAG;

    // WRITE
    int res = write(fd, ua, 5 * sizeof(char));
    printf("%02x%02x%02x%02x%02x - %d bytes written\n", ua[0], ua[1], ua[2], ua[3], ua[4], res);

    return res;
}

int sendDiscTramas(int fd, int status) {
    if (status == TRANSMITTER) {
        //Transmitter

        while (n_timeouts < datalink.numTransmissions) {
            if (!received_disc) {
                //Write disc
                message("Writting disc");
                int res_disc = write_disc(fd, status);
                if (res_disc < 0)
                    return res_disc;
                alarm(datalink.timeout);

                //Read disc
                message("Reading disc");
                read_disc(fd, status);
                alarm(0);
            } else
                break;
        }

        //Stop execution if could not stablish connection after MAX_TIMEOUTS
        if (!received_disc)
            return -1;

        //Write ua
        message("Writting ua");
        int res_ua = write_ua(fd, status);
        if (res_ua < 0)
            return res_ua;

    } else {
        //Receiver

        //Read disc
        message("Reading disc");
        read_disc(fd, status);

        while (n_timeouts < datalink.numTransmissions) {
            if (!received_ua) {
                //Write disc
                message("Writting disc");
                int res_disc = write_disc(fd, status);
                if (res_disc < 0)
                    return res_disc;
                alarm(datalink.timeout);

                //Read ua
                message("Reading ua");
                read_ua(fd, status);
                alarm(0);
            } else
                break;
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
    else
        disc[1] = A_ANS;
    disc[2] = C_DISC;
    if (status == TRANSMITTER)
        disc[3] = A_CMD ^ C_DISC;
    else
        disc[3] = A_ANS ^ C_DISC;
    disc[4] = FLAG;

    int res = write(fd, disc, 5 * sizeof(char));
    printf("%02x%02x%02x%02x%02x - %d bytes written\n", disc[0], disc[1], disc[2], disc[3], disc[4], res);

    return res;
}

void read_disc(int fd, int status) {
    unsigned char disc[STR_SIZE];
    int res;
    int n_bytes = 0;
    unsigned char read_char[1];
    read_char[0] = '\0';
    u_int8_t A;
    if (status == TRANSMITTER)
        A = A_ANS;
    else
        A = A_CMD;

    receiving_disc_state = START;
    break_read_loop = 0;

    while (!break_read_loop) {
        res = read(fd, read_char, sizeof(char));
        disc[n_bytes] = read_char[0];

        switch (receiving_disc_state) {
            case START: {
                if (read_char[0] == FLAG) {
                    receiving_disc_state = FLAG_RCV;
                    n_bytes++;
                }
                break;
            }
            case FLAG_RCV: {
                if (read_char[0] == A) {
                    receiving_disc_state = A_RCV;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    n_bytes = 1;
                    break;
                } else {
                    receiving_disc_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case A_RCV: {
                if (read_char[0] == C_DISC) {
                    receiving_disc_state = C_RCV;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    receiving_disc_state = FLAG_RCV;
                    n_bytes = 1;
                } else {
                    receiving_disc_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case C_RCV: {
                if (read_char[0] == A ^ C_DISC) {
                    receiving_disc_state = BCC_OK;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    receiving_disc_state = FLAG_RCV;
                    n_bytes = 1;
                } else {
                    receiving_disc_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case BCC_OK: {
                if (read_char[0] == FLAG) {
                    receiving_disc_state = FINISH;
                    n_bytes++;
                } else {
                    receiving_disc_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case FINISH: {
                break_read_loop = 1;
                received_disc = 1;
                break;
            }
        }
    }

    printf("%02x%02x%02x%02x%02x - %d bytes read\n", disc[0], disc[1], disc[2], disc[3], disc[4], n_bytes);
}

void cleanup(int fd) {
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    close(fd);

    message("Cleaned up terminal.");
}

int sendITramas(int fd, char *buffer, int length) {
    int res_i;
    //Reset flag
    received_i = 0;

    while (n_timeouts < datalink.numTransmissions) {
        if (!received_i) {
            //Write trama I
            message("Writting Trama I");
            res_i = write_i(fd, buffer, length);
            alarm(datalink.timeout);

            //Read trama RR
            message("Reading Trama RR");
            int rej = read_rr(fd);
            alarm(0);

            if (!rej) {
                //Change sequence number
                if (!timed_out)
                    datalink.sequenceNumber = (datalink.sequenceNumber + 1) % 2;
                timed_out = 0;
            }
        } else
            break;
    }

    //Stop execution if could not send trama I after MAX_TIMEOUTS
    if (!received_i)
        return -1;

    return res_i;
}

int receiveITramas(int fd, unsigned char *buffer) {
    //Read trama I
    message("Reading Trama I");
    int reject = 0;
    int data_bytes = read_i(fd, buffer, &reject);
    if (reject) {
        //Write trama REJ
        message("Writting Trama REJ");
        int res_rej = write_rej(fd);
    }
    else {
        //Write trama RR
        message("Writting Trama RR");
        int res_rr = write_rr(fd);

        //Change sequence number
        if (!timed_out)
            datalink.sequenceNumber = (datalink.sequenceNumber + 1) % 2;
        timed_out = 0;
    }

    return data_bytes;
}

int write_i(int fd, char *buffer, int length) {
    //Create trama
    unsigned char trama[6 + length];
    u_int8_t bcc2 = 0x00;
    trama[0] = FLAG;
    trama[1] = A_CMD;
    if (datalink.sequenceNumber)
        trama[2] = C_1;
    else
        trama[2] = C_0;
    trama[3] = A_CMD ^ trama[2];

    for (int i = 0; i < length; i++) {
        trama[4 + i] = buffer[i];
        bcc2 = bcc2 ^ buffer[i];
    }
    trama[4 + length] = bcc2;
    trama[5 + length] = FLAG;

    //Stuffing
    int new_bytes = 0;
    int nr_bytes = 6 + length;

    unsigned char *stuf = malloc(nr_bytes);

    stuf[0] = trama[0];
    stuf[1] = trama[1];
    stuf[2] = trama[2];
    stuf[3] = trama[3];
    for (int j = 4; j < 5 + length; j++) {
        if (trama[j] == FLAG) {
            nr_bytes++;
            new_bytes++;
            stuf = (char *)realloc(stuf, nr_bytes);
            stuf[j + new_bytes - 1] = ESCAPE;
            stuf[j + new_bytes] = FLAG ^ STUF;
        } else if (trama[j] == ESCAPE) {
            nr_bytes++;
            new_bytes++;
            stuf = (char *)realloc(stuf, nr_bytes);
            stuf[j + new_bytes - 1] = ESCAPE;
            stuf[j + new_bytes] = ESCAPE ^ STUF;
        } else stuf[j + new_bytes] = trama[j];
    }
    stuf[nr_bytes - 1] = FLAG;

    //Write trama I
    int res = write(fd, stuf, nr_bytes);

    for (int i = 0; i < nr_bytes-1; i++) {
        printf("%02x", stuf[i]);
    }
    printf("%02x - %d data bytes written\n", stuf[nr_bytes-1], length);

    return length;
}

int read_i(int fd, char *buffer, int *reject) {
    unsigned char trama[STR_SIZE+2] = {};
    unsigned char data[STR_SIZE+2] = {};
    int res;
    int n_bytes = 0;
    int data_bytes = 0;
    unsigned char read_char[1];
    read_char[0] = '\0';

    receiving_data_state = START_I;
    break_read_loop = 0;
    int received_second_flag = 0;

    while (!break_read_loop) {
        if (!received_second_flag) {
            res = read(fd, read_char, sizeof(char));
            if (res < 0)
                continue;
            if (res == 0)
                return res;
            trama[n_bytes] = read_char[0];
        }

        switch (receiving_data_state) {
            case START_I: {
                if (read_char[0] == FLAG) {
                    receiving_data_state = FLAG_RCV_I;
                    n_bytes++;
                }
                break;
            }
            case FLAG_RCV_I: {
                if (read_char[0] == A_CMD) {
                    receiving_data_state = A_RCV_I;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    n_bytes = 1;
                    break;
                } else {
                    receiving_data_state = START_I;
                    n_bytes = 0;
                }
                break;
            }
            case A_RCV_I: {
                if ((read_char[0] == C_0 && (!datalink.sequenceNumber)) || (read_char[0] == C_1 && datalink.sequenceNumber)) {
                    receiving_data_state = C_RCV_I;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    receiving_data_state = FLAG_RCV_I;
                    n_bytes = 1;
                    break;
                } else {
                    receiving_data_state = START_I;
                    n_bytes = 0;
                }
                break;
            }
            case C_RCV_I: {
                if (read_char[0] == A_CMD ^ C_0 || read_char[0] == A_CMD ^ C_1) {
                    receiving_data_state = BCC1_OK_I;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    receiving_data_state = FLAG_RCV_I;
                    n_bytes = 1;
                } else {
                    receiving_data_state = START_I;
                    n_bytes = 0;
                }
                break;
            }
            case BCC1_OK_I: {
                if (read_char[0] == ESCAPE) {
                    receiving_data_state = ESCAPE_RCV_I;
                } else {
                    receiving_data_state = DATA_RCV_I;
                    data[data_bytes] = trama[n_bytes];
                    n_bytes++;
                    data_bytes++;
                }
                break;
            }
            case DATA_RCV_I: {
                if (read_char[0] == FLAG) {
                    receiving_data_state = FLAG2_RCV_I;
                    received_second_flag = 1;
                    n_bytes++;
                } else if (read_char[0] == ESCAPE) {
                    receiving_data_state = ESCAPE_RCV_I;
                } else {
                    data[data_bytes] = trama[n_bytes];
                    n_bytes++;
                    data_bytes++;
                }
                break;
            }
            case FLAG2_RCV_I: {
                u_int8_t bcc = 0x00;
                for (int i = n_bytes - data_bytes - 1; i < n_bytes - 2; i++) {
                    bcc = bcc ^ trama[i];
                }
                if (trama[n_bytes - 2] == bcc) {
                    data_bytes--;
                    receiving_data_state = FINISH_I;
                } else {
                    receiving_data_state = FINISH_I;
                    *reject = 1; //possibility of sending rej message
                    data_bytes = REJECT_DATA;
                }
                break;
            }
            case FINISH_I: {
                break_read_loop = 1;
                received_i = 1;
                break;
            }
            case ESCAPE_RCV_I: {
                receiving_data_state = DATA_RCV_I;
                trama[n_bytes] = read_char[0] ^ STUF;
                data[data_bytes] = trama[n_bytes];
                data_bytes++;
                n_bytes++;
                break;
            }
        }
    }

    //New trama
    if (nr_tramaI == data[1] || (!control_start && data[1] == '\0')) {
        //bcc2 wrong, then reject
        if (*reject)  
            return REJECT_DATA;

        //bcc good, then accept
        memcpy(buffer, data, STR_SIZE);
        if (!control_start) 
            nr_tramaI = (nr_tramaI + 1) % 256;
        control_start = 0;
    }
    else { //Duplicated trama, then send rr
        *reject = 0;
        data_bytes = REJECT_DATA;
    }

    /*for (int i = 0; i < n_bytes-1; i++) {
        printf("%02x", trama[i]);
    }
    printf("%02x - %d data bytes read\n", trama[n_bytes-1], data_bytes);*/

    return data_bytes;
}

int write_rr(int fd) {
    //Create trama rr
    unsigned char rr[5];
    rr[0] = FLAG;
    rr[1] = A_CMD;
    if (datalink.sequenceNumber)
        rr[2] = C_RR0;
    else
        rr[2] = C_RR1;
    if (datalink.sequenceNumber)
        rr[3] = A_CMD ^ C_RR0;
    else
        rr[3] = A_CMD ^ C_RR1;
    rr[4] = FLAG;

    int res = write(fd, rr, 5 * sizeof(char));
    printf("%02x%02x%02x%02x%02x - %d bytes written\n", rr[0], rr[1], rr[2], rr[3], rr[4], res);

    return res;
}

int read_rr(int fd) {
    unsigned char rr[STR_SIZE];
    memset(rr, '\0', STR_SIZE);
    u_int8_t c_rr, c_rej;
    int res;
    int rej = 0;
    int n_bytes = 0;
    unsigned char read_char[1];

    receiving_rr_state = START;
    break_read_loop = 0;

    while (!break_read_loop) {
        if (receiving_rr_state != FINISH_I) {
            res = read(fd, read_char, sizeof(char));
            rr[n_bytes] = read_char[0];
        }

        switch (receiving_rr_state) {
            case START: {
                if (read_char[0] == FLAG) {
                    receiving_rr_state = FLAG_RCV;
                    n_bytes++;
                }
                break;
            }
            case FLAG_RCV: {
                if (read_char[0] == A_CMD) {
                    receiving_rr_state = A_RCV;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    n_bytes = 1;
                    break;
                } else {
                    receiving_rr_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case A_RCV: {
                if (datalink.sequenceNumber) {
                    c_rr = C_RR0;
                    c_rej = C_REJ1;
                }
                else {
                    c_rr = C_RR1;
                    c_rej = C_REJ0;
                }
                if (read_char[0] == c_rr) {
                    receiving_rr_state = C_RCV;
                    n_bytes++;
                } else if (read_char[0] == c_rej) {
                    receiving_rr_state = C_RCV;
                    rej = 1;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    receiving_rr_state = FLAG_RCV;
                    n_bytes = 1;
                } else {
                    receiving_rr_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case C_RCV: {
                if ((read_char[0] == A_CMD ^ c_rr) || (read_char[0] == A_CMD ^ c_rej)) {
                    receiving_rr_state = BCC_OK;
                    n_bytes++;
                } else if (read_char[0] == FLAG) {
                    receiving_rr_state = FLAG_RCV;
                    n_bytes = 1;
                } else {
                    receiving_rr_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case BCC_OK: {
                if (read_char[0] == FLAG) {
                    receiving_rr_state = FINISH;
                    n_bytes++;
                } else {
                    receiving_rr_state = START;
                    n_bytes = 0;
                }
                break;
            }
            case FINISH: {
                break_read_loop = 1;
                received_i = 1;
                break;
            }
        }
    }

    printf("%02x%02x%02x%02x%02x - %d bytes read\n", rr[0], rr[1], rr[2], rr[3], rr[4], n_bytes);

    return rej;
}

int write_rej(int fd) {
    //Create trama rej
    unsigned char rej[5];
    rej[0] = FLAG;
    rej[1] = A_CMD;
    if (datalink.sequenceNumber)
        rej[2] = C_REJ1;
    else
        rej[2] = C_REJ0;
    if (datalink.sequenceNumber)
        rej[3] = A_CMD ^ C_REJ1;
    else
        rej[3] = A_CMD ^ C_REJ0;
    rej[4] = FLAG;

    int res = write(fd, rej, 5 * sizeof(char));
    printf("%02x%02x%02x%02x%02x - %d bytes written\n", rej[0], rej[1], rej[2], rej[3], rej[4], res);

    return res;
}

