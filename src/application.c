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
    ctrl_packet start_packet, end_packet;
    data_packet data_packet;
    make_packets(fd_file, &start_packet, &end_packet, &data_packet);

    // Fragments of file to send
    unsigned char fragment[FRAG_SIZE];
    unsigned char buffer[STR_SIZE];
    int numbytes, size_packet, n_chars_written;

    // Write information
    message("Started llwrite");

    // Send START packet
    memset(buffer, '\0', STR_SIZE);
    packet_to_array(&start_packet, buffer);
    n_chars_written = llwrite(application.fd_port, buffer, START_SIZE);
    LTZ_RET(n_chars_written)


    // Read fragments and send them one by one
    memset(buffer, '\0', STR_SIZE);
    while ((numbytes = read(fd_file, fragment, FRAG_SIZE)) != 0) {
      LTZ_RET(numbytes)

      // Send DATA packets
      data_packet.sequence_number = (data_packet.sequence_number + 1) % 256;
      data_packet.nr_bytes2 = numbytes/256;
      data_packet.nr_bytes1 = numbytes%256;
      memcpy(data_packet.data, fragment, numbytes);
      packet_to_array(&data_packet, buffer);

      n_chars_written = llwrite(application.fd_port, buffer, DATA_SIZE);
      LTZ_RET(n_chars_written)
      memset(buffer, '\0', STR_SIZE);
    }
    close(fd_file);

    // Send END packet
    packet_to_array(&end_packet, buffer);
    n_chars_written = llwrite(application.fd_port, buffer, END_SIZE);
    LTZ_RET(n_chars_written)
  }
  else {
    // RECEIVER

    // Fragments of file to read
    ctrl_packet start_packet, end_packet;
    data_packet data_packet;
    unsigned char read_buffer[STR_SIZE];
    int n_chars_read;

    // Receive information
    message("Started llread");

    // Read START packet
    memset(read_buffer, '\0', STR_SIZE);
    n_chars_read = llread(application.fd_port, read_buffer);
    LTZ_RET(n_chars_read)
    array_to_packet(&start_packet, read_buffer);

    // Create file
    int fd_file = open(start_packet.name.value, O_WRONLY | O_CREAT | O_TRUNC |O_APPEND , 0664);
    LTZ_RET(fd_file)

    // Read fragments
    memset(read_buffer, '\0', STR_SIZE);
    while ((n_chars_read = llread(application.fd_port, read_buffer)) != 0) {
      LTZ_RET(n_chars_read)

      if(read_buffer[0] == 1){
        array_to_packet(&data_packet, read_buffer);
        write(fd_file, data_packet.data, data_packet.nr_bytes2*256+data_packet.nr_bytes1);
      }
      else {
        array_to_packet(&end_packet, read_buffer);
        break;
      }
      memset(read_buffer, '\0', STR_SIZE);
    }

    close(fd_file);
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
