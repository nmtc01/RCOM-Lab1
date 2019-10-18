/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Emitter main file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include "emitter.h"

volatile sig_atomic_t STOP = FALSE;
volatile sig_atomic_t n_timeouts = 0;
volatile sig_atomic_t read_answer = 0;
volatile sig_atomic_t received_ua = 0;
volatile sig_atomic_t received_disc = 0;
volatile sig_atomic_t break_read_loop = 0;
enum state receiving_ua_state;
enum state receiving_disc_state;

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

  message("Emitter timed out.");
}

int main(int argc, char **argv) {
  int fd;
  struct termios oldtio;
  struct termios newtio;
  char ua[STR_SIZE];
  char disc[STR_SIZE];

  setup(argc, argv);
  open_port(argv, &fd);
  set_flags(&oldtio, &newtio, &fd);
  (void) signal(SIGALRM, timeout_handler);

  //STABLISH CONNECTION
  while(n_timeouts < MAX_TIMEOUTS){
    if (!received_ua) {
      //Write set
      message("Writting set");
      int res_set = write_set(fd);
      alarm(3);

      //Read ua
      message("Reading ua");
      read_ua(fd, ua);
	    alarm(0);
    }
    else break;
  }

  //Stop execution if could not stablish connection after MAX_TIMEOUTS
  if (!received_ua)
      return -1;

  //Reset number of timeouts and buf
  n_timeouts = 0;

  //DISCONNECT
  while(n_timeouts < MAX_TIMEOUTS){
    if (!received_disc) {
      //Write disc
      message("Writting disc");
      int res_disc = write_disc(fd);
      alarm(3);

      //Read disc
      message("Reading disc");
      read_disc(fd, disc);
      alarm(0);
    }
    else break;
  }

  //Stop execution if could not stablish connection after MAX_TIMEOUTS
  if (!received_disc)
      return -2;

  //Write ua
  message("Writting ua");
  int res_ua = write_ua(fd);

  //Clean up
  cleanup(&oldtio, fd);

  return 0;
}

void setup(int argc, char **argv){
	if ((argc < 2) || ((strcmp("/dev/ttyS0", argv[1]) != 0) && (strcmp("/dev/ttyS1", argv[1]) != 0))) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }
  (void) signal(SIGALRM, timeout_handler);

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

void read_ua(int fd, unsigned char *answer) {
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

int write_disc(int fd) {
  //Create trama DISC
  unsigned char disc[5];
  disc[0] = FLAG;
  disc[1] = A_CMD;
  disc[2] = C_DISC;
  disc[3] = A_CMD ^ C_DISC;
  disc[4] = FLAG;

  int res = write(fd, disc, 5*sizeof(char));
  printf("%x%x%x%x%x - %d bytes written\n", disc[0], disc[1], disc[2], disc[3], disc[4], res);

  return res;
}

void read_disc(int fd, unsigned char *request) {
  int res;
  int n_bytes = 0;
  char read_char[1];
  read_char[0] = '\0';

  receiving_disc_state = START;
  break_read_loop = 0;

  while (!break_read_loop) {
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
            if (read_char[0] == A_ANS^C_DISC) {
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

  printf("%x%x%x%x%x - %d bytes read\n", request[0], request[1], request[2], request[3], request[4], n_bytes);
}

int write_ua(int fd) {

  //Create trama DISC
  unsigned char ua[5];
  ua[0] = FLAG;
  ua[1] = A_ANS;
  ua[2] = C_UA;
  ua[3] = A_ANS ^ C_UA;
  ua[4] = FLAG;

  int res = write(fd, ua, 5*sizeof(char));
  printf("%x%x%x%x%x - %d bytes written\n", ua[0], ua[1], ua[2], ua[3], ua[4], res);

  return res;
}

void cleanup(struct termios *oldtio_ptr, int fd){
	if (tcsetattr(fd, TCSANOW, oldtio_ptr) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
  close(fd);

	message("Cleaned up terminal.");
}
/*
void create_message(int fd_port){
  int fd_file, res, out_size;
  char buf[BUF_SIZE];
  char* buf_out;

  fd_file = open(FILE_TO_SEND, O_RDONLY | O_NONBLOCK);

  // Read file into buffer
  while(1){
    res = read(fd_file, buf, BUF_SIZE);
    if(res < 0){
      message("Error reading file.");
      // Terminate then exit
      exit(1);
    }
    else if (res == 0){
      message("Finished reading file.");
      break;
    }

    // Check errors in message
    check_message(buf, buf_out, &out_size);

    // Write message to port
    write_msg(fd_port, buf_out, out_size);
  }

}

void check_message(char* buf, char* buf_out, int* out_size){
  int i = 0, j = 0, n_flags = 0;
  char bcc = 0;


  // Count number of bytes to change
  // While also calculating BCC
  while(i < BUF_SIZE){
    bcc = bcc ^ buf[i];
    if(buf[i] == FLAG){
      n_flags++;
    }
  }

  // Allocate buf_out


  // Recreate message

  while(i < BUF_SIZE){
    if(buf[i] == FLAG){
      buf_out[]
    }else{
      buf_out[j] = buf[i];
    }
    i++; j++;
  }
}

void write_msg(int fd_port, char* buf, int size){

}
*/

