/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Application header file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include "datalink.h"

#define FILE_TO_SEND "images/pinguim.gif"
#define CNTRL_START 2
#define CNTRL_END 3

typedef struct data_packet {
    char control;          // = 1 for data
    char sequence_number;  // number of data_packet
    char nr_bytes2;
    char nr_bytes1;  // K = 256 * nr_bytes2 + nr_bytes1
    char* data;      // data with K bytes
} data_packet;

typedef struct tlv_packet {
    char type;    /* type 0 - size of file
                                            1 - name of file
                                          ... - to be defined */
    char length;  // size of value
    char* value;  // value
} tlv_packet;

typedef struct ctrl_packet {
    char control;     /* 2 - start
                                               3 - end   */
    tlv_packet size;  // size of file
    tlv_packet name;  // name of file
} ctrl_packet;

//Application struct
struct appLayer {
    int fd_port;
    int status;
};

//Open
void setup(int argc, char** argv);
int llopen(int port, int status);

//Write
int llwrite(int fd, char* packet, int length);

//Read
int llread(int fd, char* buffer);

//Close
int llclose(int fd, int status);

void make_packets(int fd_file, ctrl_packet* start_packet, ctrl_packet* end_packet, data_packet* data_packet);
void packet_to_array(void* packet_void_ptr, char* buffer);