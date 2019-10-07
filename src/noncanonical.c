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

int main(int argc, char **argv) {
  int fd, c, res;
  struct termios oldtio, newtio;

  if ((argc < 2) || ((strcmp("/dev/ttyS0", argv[1]) != 0) && (strcmp("/dev/ttyS1", argv[1]) != 0))) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0) {
    perror(argv[1]);
    exit(-1);
  }

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

  printf("New termios structure set\n");
  char buf[STR_SIZE], read_char[2];
  int res;

  /*char msg[255];
  int i = 0;

while (STOP==FALSE) {
res = read(fd,buf,1);
buf[res]=0;
    msg[i] = buf[0];
    i++;
printf("%s", buf);
    if(buf[0] == '\0') {
          printf("\n");
          msg[i+1] = '\0';
          STOP=TRUE;
    }
}

  write(fd, msg, (strlen(msg)+1)*sizeof(char));*/

  while (STOP == FALSE) {
    res = read(fd, buf, 1);
  }

  /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o
  */

  tcsetattr(fd, TCSANOW, &oldtio);
  close(fd);
  return 0;
}
