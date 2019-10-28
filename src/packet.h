#include "datalink.h"

#define FILE_TO_SEND "images/pinguim.gif"
#define CNTRL_START 2
#define CNTRL_END 3

typedef struct data_packet {
    unsigned char control;          // = 1 for data
    unsigned char sequence_number;  // number of data_packet
    unsigned char nr_bytes2;
    unsigned char nr_bytes1;  // K = 256 * nr_bytes2 + nr_bytes1
    unsigned char* data;      // data with K bytes
} data_packet;

typedef struct tlv_packet {
    unsigned char type;    /* type 0 - size of file
                     type 1 - name of file */
    unsigned char length;  // size of value
    char* value;  // value
} tlv_packet;

typedef struct ctrl_packet {
    unsigned char control;     /* 2 - start
                         3 - end   */
    tlv_packet size;  // size of file
    tlv_packet name;  // name of file
} ctrl_packet;

void transmitter_packets(int fd_file, ctrl_packet* start_packet, ctrl_packet* end_packet, data_packet* data_packet);
void receiver_packets(ctrl_packet* start_packet, ctrl_packet* end_packet, data_packet* data_packet);
void packet_to_array(void* packet_void_ptr, char* buffer);
void array_to_packet(void *packet_void_ptr, char *buffer);
