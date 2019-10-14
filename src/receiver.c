/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Receiver main file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include "receiver.h"

enum state receiving_set_state;
enum state receiving_disc_state;
enum state receiving_ua_state;
volatile sig_atomic_t break_read_loop = 0;
volatile sig_atomic_t n_timeouts = 0;
int received_ua = 0;

void timeout_handler(){
  if (receiving_ua_state != FINISH) {
    received_ua = 0;
    receiving_ua_state = START;
    break_read_loop = 1;
    n_timeouts++;
  }

  message("Receiver timed out.");
}

int main(int argc, char **argv) {
  int fd;
  struct termios oldtio;
  struct termios newtio;

  setup(argc, argv);
  open_port(argv, &fd);
  set_flags(&oldtio, &newtio, &fd);
  (void) signal(SIGALRM, timeout_handler);

  char set[STR_SIZE];
  char disc[STR_SIZE];
  char ua[STR_SIZE];

  //STABLISH CONNECTION
  //Read set
  message("Reading set");
  int n_set = read_set(&fd, set);
  //Write ua
  message("Writting ua");
  int res_ua = write_ua(&fd, n_set);

  //DISCONNECT
  //Read disc
  message("Reading disc");
  int n_disc = read_disc(&fd, disc); 
  
  while(n_timeouts < MAX_TIMEOUTS) {
    if (!received_ua) {
        //Write disc
        message("Writting disc");
        int res_disc = write_disc(&fd, n_disc); 
        alarm(3);
        
        //Read ua
        message("Reading ua");
        int n_ua = read_ua(&fd, ua);
        alarm(0);
    }
    else break;
  }

  cleanup(&oldtio, &fd);
  return 0;
}

void setup(int argc, char **argv){
	if ((argc < 2) || ((strcmp("/dev/ttyS0", argv[1]) != 0) && (strcmp("/dev/ttyS1", argv[1]) != 0))) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }
	message("Checked arguments");
}

void open_port(char **argv, int *fd_ptr){

	*fd_ptr = open(argv[1], O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (*fd_ptr < 0) {
    perror(argv[1]);
    exit(-1);
  }
	message("Opened serial port.");
}

void set_flags(struct termios *oldtio_ptr, struct termios *newtio_ptr, int *fd_ptr){
  if (tcgetattr(*fd_ptr, oldtio_ptr) == -1) {
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

  tcflush(*fd_ptr, TCIOFLUSH);
  if (tcsetattr(*fd_ptr, TCSANOW, newtio_ptr) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  message("Terminal flags set.");
}

int read_set(int *fd_ptr, unsigned char *request) {
  char read_char[2];
  int n_bytes = 0;
  int fd = *fd_ptr;
  int res;
  int received_set = 0;
  receiving_set_state = START;

  // READ
  while (!received_set) {
    res = read(fd, read_char, sizeof(char));               
    request[n_bytes] = read_char[0];     
    
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
        if (read_char[0] == A_ANS) {
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
        if (read_char[0] == A_ANS^C_SET) {
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
  printf("%x%x%x%x%x - %d bytes read\n", request[0],request[1],request[2],request[3],request[4], n_bytes);

  return n_bytes;
}

int write_ua(int *fd_ptr, int n_bytes) {
  int fd = *fd_ptr;

  //Create trama UA
  unsigned char ua[5];
  ua[0] = FLAG;
  ua[1] = A_ANS;
  ua[2] = C_UA;
  ua[3] = A_ANS ^ C_UA;
  ua[4] = FLAG;

  // WRITE
  int res = write(fd, ua, n_bytes);
  printf("%x%x%x%x%x - %d bytes written\n", ua[0],ua[1],ua[2],ua[3],ua[4], res);

  return res;
}

void cleanup(struct termios *oldtio_ptr, int *fd_ptr){
	int fd = *fd_ptr;

	if (tcsetattr(fd, TCSANOW, oldtio_ptr) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
  close(fd);

	message("Cleaned up terminal.");
}

int read_disc(int *fd_ptr, unsigned char *request) {
  int n_bytes = 0;
  int fd = *fd_ptr;
  int res;
  int received_disc = 0;

  char read_char[1];
  read_char[0] = '\0';

  receiving_disc_state = START;

  // READ
  while (!received_disc) {
    res = read(fd, read_char, sizeof(char));               
    request[n_bytes] = read_char[0];     
    
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
        if (read_char[0] == A_ANS) {
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
        if (read_char[0] == A_ANS^C_SET) {
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
        received_disc = 1;
        break;
      }
    }
  }
  printf("%x%x%x%x%x - %d bytes read\n", request[0],request[1],request[2],request[3],request[4], n_bytes);

  return n_bytes;
}

int write_disc(int *fd_ptr, int n_bytes) {
  int fd = *fd_ptr;
  
  //Create trama UA
  unsigned char disc[5];
  disc[0] = FLAG;
  disc[1] = A_CMD;
  disc[2] = C_DISC;
  disc[3] = A_CMD ^ C_DISC;
  disc[4] = FLAG;

  // WRITE
  int res = write(fd, disc, n_bytes);
  printf("%x%x%x%x%x - %d bytes written\n", disc[0],disc[1],disc[2],disc[3],disc[4], res);

  return res;
}

int read_ua(int *fd_ptr, unsigned char *answer) {
  int fd = *fd_ptr;
  int res;
  int n_bytes = 0;

  char read_char[1];
  read_char[0] = '\0';

  receiving_ua_state = START;
  break_read_loop = 0;

  while (!break_read_loop) {
    res = read(fd, read_char, sizeof(char));	
    answer[n_bytes] = read_char[0];
    
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

  printf("%x%x%x%x%x - %d bytes read\n", answer[0], answer[1], answer[2], answer[3], answer[4], n_bytes);
}

void message(char* message){printf("!--%s\n", message);}





