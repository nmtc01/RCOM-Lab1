/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Emitter main file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1
#define FALSE 0
#define TRUE 1

// FLAGS
#define FLAG 0x7E
#define A_CMD 0x03
#define A_ANS 0x01
#define C 0x03

// SIZES
#define STR_SIZE 255

volatile int STOP = FALSE;

void message(char* message);
void setup(int argc, char **argv);
void open_port(char **argv, int *fd_ptr);
void set_flags(struct termios *oldtio_ptr, struct termios *newtio_ptr, int *fd_ptr);
void write_msg();
void read_msg();
void cleanup(struct termios *oldtio_ptr, int *fd_ptr);

int main(int argc, char **argv) {
  int fd, c;
  struct termios oldtio, newtio;

	setup(argc, argv);
	open_port(argv, &fd);
	set_flags(&oldtio, &newtio, &fd);

  char buf[STR_SIZE];
	char* read_char;
  int res, n_bytes = 0;

	// WRITE
  memset(buf, '\0', STR_SIZE);
	gets(buf); buf[strlen(buf)] = '\0';
  res = write(fd, buf, (strlen(buf) + 1) * sizeof(char));
  printf("'%s' - %d bytes written\n", buf, res);

	// READ
  memset(buf, '\0', STR_SIZE);
  while (STOP == FALSE) {
    res = read(fd, read_char, sizeof(char));
		buf[n_bytes] = *read_char;
    n_bytes++;
    if (*read_char == '\0') STOP = TRUE;
  }
  printf("'%s' - %d bytes read\n", buf, n_bytes);

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
	int fd; &fd = fd_ptr;

	fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0) {
    perror(argv[1]);
    exit(-1);
  }
	message("Opened serial port.");
}

void set_flags(struct termios *oldtio_ptr, struct termios *newtio_ptr, int *fd_ptr){
	int fd; &fd = fd_ptr;
  struct termios oldtio; &oldtio = oldtio_ptr;
	struct termios newtio; &newtio = newtio_ptr;

  if (tcgetattr(fd, &oldtio) == -1) {
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
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

void write_msg();
void read_msg();
void cleanup(struct termios *oldtio_ptr, int *fd_ptr){
	if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
  close(fd);

	message("Cleaned up terminal.");
}

void message(char* message){printf("!--%s\n", message);}