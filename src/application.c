/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Application main file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include "application.h"

int main(int argc, char **argv) {

  // Validate arguments
  setup(argc, argv);

  // Application struct
  struct appLayer application;
  if (strcmp(argv[1], "transmitter") == 0)
    application.status = TRANSMITTER;
  else
    application.status = RECEIVER;

  // Port
  int port;
  if ((strcmp("/dev/ttyS0", argv[2]) == 0))
    port = COM1;
  else
    port = COM2;

  // Stablish communication
  message("Started llopen");
  application.fd_port = llopen(port, application.status);
  if (application.fd_port < 0) {
    perror("llopen");
    return -1;
  }

  // Main Communication

  if (application.status == TRANSMITTER) {
    // Transmitter
    // Open file
    int fd_file = open(FILE_TO_SEND, O_RDONLY | O_NONBLOCK);
    if (fd_file < 0) {
      perror("Opening File");
      return -1;
    }

    // Create packets
    struct stat file_stat;
    if (fstat(fd_file, &file_stat) < 0) {
      message("Error reading file. Exitting.");
      exit(2);
    }

    ctrl_packet start_packet, end_packet;
    tlv_packet size_tlv, name_tlv;
    data_packet data_packet;

    size_tlv.type = 0;
    size_tlv.length = sizeof(int);
    size_tlv.value = file_stat.st_size;

    name_tlv.type = 1;
    name_tlv.value = basename(FILE_TO_SEND);
    name_tlv.length = sizeof(char) * strlen(name_tlv.value);

    data_packet.control = 1;
    data_packet.sequence_number = 255;
    data_packet.nr_bytes2 = FRAG_SIZE / 256;
    data_packet.nr_bytes1 = FRAG_SIZE % 256;

    start_packet.control = 2;
    start_packet.size = size_tlv;
    start_packet.name = name_tlv;

    end_packet.control = 3;
    end_packet.size = size_tlv;
    end_packet.name = name_tlv;

    // Fragments of file to send
    unsigned char send_frag[FRAG_SIZE];
    int numbytes;

    // Write information
    message("Started llwrite");

    // TODO: send start packet

    // Read fragments and send them one by one
    while ((numbytes = read(fd_file, send_frag, FRAG_SIZE)) != 0) {
      if (numbytes < 0) {
        perror("readFile");
        return -1;
      }

      data_packet.sequence_number = (data_packet.sequence_number + 1) % 256;
      data_packet.data = send_frag;

      int n_chars_written = llwrite(application.fd_port, (void*)data_packet, FRAG_SIZE);
      if (n_chars_written < 0) {
        perror("llwrite");
        return -1;
      }
    }

    // TODO: send end packet


  } else {
    // Receive information
    message("Started llread");

    // Fragments of file to read
    unsigned char receive_frag[FRAG_SIZE];
    int n_chars_read;

    // Read fragments
    while ((n_chars_read = llread(application.fd_port, receive_frag)) != 0) {
      if (n_chars_read < 0) {
        perror("llread");
        return -1;
      }
    }
  }

  // Finish communication
  message("Started llclose");
  if (llclose(application.fd_port, application.status) < 0) {
    perror("llclose");
    return -1;
  }

  return 0;
}

void setup(int argc, char **argv) {
  if ((argc != 3) || ((strcmp("/dev/ttyS0", argv[2]) != 0) &&
                      (strcmp("/dev/ttyS1", argv[2]) != 0))) {
    printf("Usage:\tnserial transmitter|receiver SerialPort\n\tex: nserial "
           "transmitter /dev/ttyS0\n");
    exit(1);
  }

  message("Checked arguments");
}

int llopen(int port, int status) {

  // Open serial port
  int fd = open_port(port);
  // Check errors
  if (fd < 0)
    return fd;

  // Tramas set and ua
  int res = sendStablishTramas(fd, status);
  // Check errors
  if (res < 0)
    return res;

  return fd;
}

int llwrite(int fd, void* packet_void_ptr, int length) {
  char* buffer;
  data_packet* data_packet_ptr = (data_packet*)packet_void_ptr;
  ctrl_packet* ctrl_packet_ptr = (ctrl_packet*)packet_void_ptr;

  switch (data_packet_ptr->control) {
  case 1:
    buffer = malloc(sizeof(char) * (4 + data_packet_ptr->nr_bytes2 * 256 + data_packet_ptr->nr_bytes1));
    buffer[0] = data_packet_ptr->control;
    buffer[1] = data_packet_ptr->sequence_number;
    buffer[2] = data_packet_ptr->nr_bytes2;
    buffer[3] = data_packet_ptr->nr_bytes1;
    strcpy((buffer + 4), data_packet_ptr->data);
    break;

  case 2:
    buffer = malloc(sizeof(char) * (( 2 + ctrl_packet_ptr->size.length) + ( 2 + ctrl_packet_ptr->name.length) + 1) );
    buffer[0] = ctrl_packet_ptr->control;
    buffer[1] = ctrl_packet_ptr->size.type;
    buffer[2] = ctrl_packet_ptr->size.length;
    strcpy((buffer + 3), ctrl_packet_ptr->size.value);
    buffer[3 + ctrl_packet_ptr->size.length] = ctrl_packet_ptr->name.type;
    buffer[4 + ctrl_packet_ptr->size.length] = ctrl_packet_ptr->name.length;
    strcpy((buffer + 5 + ctrl_packet_ptr->size.length), ctrl_packet_ptr->name.value);
    break;

  case 3:
    buffer = malloc(sizeof(char) * (( 2 + ctrl_packet_ptr->size.length) + ( 2 + ctrl_packet_ptr->name.length) + 1) );
    buffer[0] = ctrl_packet_ptr->control;
    buffer[1] = ctrl_packet_ptr->size.type;
    buffer[2] = ctrl_packet_ptr->size.length;
    strcpy((buffer + 3), ctrl_packet_ptr->size.value);
    buffer[3 + ctrl_packet_ptr->size.length] = ctrl_packet_ptr->name.type;
    buffer[4 + ctrl_packet_ptr->size.length] = ctrl_packet_ptr->name.length;
    strcpy((buffer + 5 + ctrl_packet_ptr->size.length), ctrl_packet_ptr->name.value);
    break;

  default:
    break;
  }

  int nr_chars = sendITramas(fd, buffer, length);
  if (nr_chars < 0)
    return -1;

  return nr_chars;
}

int llread(int fd, char *buffer) {
  int nr_chars = receiveITramas(fd, buffer);

  return nr_chars;
}

int llclose(int fd, int status) {
  int res = sendDiscTramas(fd, status);
  if (res < 0)
    return res;

  cleanup(fd);

  return 1;
}
