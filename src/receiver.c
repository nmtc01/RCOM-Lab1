/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Receiver main file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include "receiver.h"

enum state receiving_set_state;

int main(int argc, char **argv) {
  int fd;
  struct termios oldtio;
  struct termios newtio;

  setup(argc, argv);
  open_port(argv, &fd);
  set_flags(&oldtio, &newtio, &fd);

  char buf[STR_SIZE];

  //READ
  int n_bytes = read_msg(&fd, buf);

  //WRITE
  int res = write_msg(&fd, n_bytes);

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

int read_msg(int *fd_ptr, unsigned char *request) {
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
  printf("%x%x%x%x%x - %d bytes read\n", request[0],request[1],request[2],request[3],request[4], n_bytes);

  return n_bytes;
}

int write_msg(int *fd_ptr, int n_bytes) {
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

void message(char* message){printf("!--%s\n", message);}

