/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Receiver main file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1
#define FALSE 0
#define TRUE 1

// FLAGS
#define FLAG 0x7E
#define A_ANS 0x03
#define A_CMD 0x01
#define C_SET 0x07

// SIZES
#define STR_SIZE 255

volatile int STOP = FALSE;

void message(char* message);
void setup(int argc, char **argv);
void open_port(char **argv, int *fd_ptr);
void set_flags(struct termios *oldtio_ptr, struct termios *newtio_ptr, int *fd_ptr);
int read_msg(int *fd_ptr, char *buf);
int write_msg(int *fd_ptr, char *buf, int n_bytes);
void cleanup(struct termios *oldtio_ptr, int *fd_ptr);

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
  int res = write_msg(&fd, buf, n_bytes);

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

int read_msg(int *fd_ptr, char *buf) {
  char read_char[2];
  int n_bytes = 0;
  int fd = *fd_ptr;
  int res;

  // READ
  memset(buf, '\0', STR_SIZE);
  while (STOP == FALSE) {
    res = read(fd, read_char, sizeof(char));
	  read_char[1] = '\0';
    buf[n_bytes] = read_char[0];
    n_bytes++;
    if (read_char[0] == '\0') STOP = TRUE;
  }
  printf("'%s' - %d bytes read\n", buf, n_bytes);

  return n_bytes;
}

int write_msg(int *fd_ptr, char *buf, int n_bytes) {
  int res;
  int fd = *fd_ptr;

  res = write(fd, buf, n_bytes);
  printf("'%s' - %d bytes written\n", buf, res);

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