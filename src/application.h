/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Application header file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include "packet.h"

#define START_SIZE      (5 + start_packet.size.length + start_packet.name.length)
#define END_SIZE        (5 + end_packet.size.length + end_packet.name.length)
#define DATA_SIZE       (4 + data_packet.nr_bytes2*256 + data_packet.nr_bytes1)
#define LTZ_RET(n)      if((n) < 0){ return -1;}

//Application struct
typedef struct appLayer {
    int fd_port;
    int status;
	int file_size;
} appLayer;


void setup(int argc, char** argv, appLayer *application, int *port);
int transmitter(appLayer *application);
int receiver(appLayer *application);
