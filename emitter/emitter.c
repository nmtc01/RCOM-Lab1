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

int main(int argc, char **argv) {
  int fd;
  struct termios oldtio;
  struct termios newtio;
  char ans[STR_SIZE];

  setup(argc, argv);
  open_port(argv, &fd);
  set_flags(&oldtio, &newtio, &fd);

  while(n_timeouts < MAX_TIMEOUTS){
    // WRITE
    message("Writting");
    int res = write_set(&fd);
    alarm(3);

    // READ
    message("Reading");
    int n_bytes = read_ua(&fd, ans);
  }
  cleanup(&oldtio, &fd);

  return 0;
}

void timeout_handler(){
  n_timeouts++;
  message("Emitter timed out.");
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

	*fd_ptr = open(argv[1], O_RDWR | O_NOCTTY);
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

int write_set(int *fd_ptr) {
  int fd = *fd_ptr;

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

int read_ua(int *fd_ptr, unsigned char *answer) {
  int fd = *fd_ptr;
  int res;
  char read_char[2];
  int n_bytes = 0;
  //memset(answer, '\0', STR_SIZE);

  while (STOP == FALSE) {									//TODO
    res = read(fd, read_char, sizeof(char));				//NEED TO SEE STOP CONDITION AHEAD WITH STATE MACHINE
	//read_char[1] = '\0';									//NOW IT'S GOOD ENOUGH
	answer[n_bytes] = read_char[0];
    n_bytes++;
    //if (read_char[0] == '\0') STOP = TRUE;
    if (n_bytes == 5) STOP = TRUE;
  }

  printf("%x%x%x%x%x - %d bytes read\n", answer[0], answer[1], answer[2], answer[3], answer[4], n_bytes);

  return n_bytes;
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
