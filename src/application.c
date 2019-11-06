/* RCOM Laboratorial Work
 * Joao Campos and Nuno Cardoso
 * Application main file
 * RS-232 Serial Port
 *
 * 07/10/2019
 */

#include "application.h"

//Counting time
clock_t start, end;
struct tms t;
long ticks;

int main(int argc, char **argv) {
  message("Starting program");
  // Validate arguments
  int port;
  appLayer application;
  setup(argc, argv, &application, &port);

  // Stablish communication
  message("Started llopen");
  application.fd_port = llopen(port, application.status);
  LTZ_RET(application.fd_port)

  //Efficiency stuff
  srand(time(NULL));
  ticks = sysconf(_SC_CLK_TCK);

  // Main Communication
  if (application.status == TRANSMITTER) {
    LTZ_RET(transmitter(&application))
  } else {
    LTZ_RET(receiver(&application))
  }

  // Finish counting time
  end = times(&t);

  // Finish communication
  message("Started llclose");
  if (llclose(application.fd_port, application.status) < 0) {
    perror("llclose");
    return -1;
  }

  // Show statistics
  message("Show Statistics");
  printf("Error probability: \t1/%d\n", ERROR_PROB);
  printf("File size: \t\t%d\n", application.file_size);
  printf("File submission time:  \t%4.3f\n",(double)(end-start)/ticks);
  printf("Rate (R): \t\t%f\n", (application.file_size*8)/((double)(end-start)/ticks));
  printf("Baud Rate (C):  \t%f\n", BAUD_VALUE);
  printf("S: \t\t\t%f\n", ((application.file_size*8)/((double)(end-start)/ticks))/38400.0);

  message("Finishing program");
  return 0;
}

void setup(int argc, char **argv, appLayer *application, int *port) {
  if ((argc != 3) || ((strcmp("/dev/ttyS0", argv[2]) != 0) &&
                      (strcmp("/dev/ttyS1", argv[2]) != 0) &&
                      (strcmp("/dev/ttyS2", argv[2]) != 0) &&
                      (strcmp("/dev/ttyS3", argv[2]) != 0) &&
                      (strcmp("/dev/ttyS4", argv[2]) != 0))) {
    printf("Usage:\tnserial transmitter|receiver SerialPort\n\tex: nserial "
           "transmitter /dev/ttyS0\n");
    exit(1);
  }

  // Application struct
  if (strcmp(argv[1], "transmitter") == 0)
    application->status = TRANSMITTER;
  else
    application->status = RECEIVER;

  // Port
  if ((strcmp("/dev/ttyS0", argv[2]) == 0))
    *port = COM1;
  else if ((strcmp("/dev/ttyS1", argv[2]) == 0))
    *port = COM2;
  else if ((strcmp("/dev/ttyS2", argv[2]) == 0))
    *port = COM3;
  else if ((strcmp("/dev/ttyS3", argv[2]) == 0))
    *port = COM4;
  else
    *port = COM5;
}

int transmitter(appLayer *application) {
  char file_to_send[255];
  printf("Input a file to send: ");
  scanf("%s", file_to_send);

  // Open file
  int fd_file = open(file_to_send, O_RDONLY | O_NONBLOCK);
  if (fd_file < 0) {
    perror("Opening File");
    return -1;
  }

  // Create packets
  ctrl_packet start_packet, end_packet;
  data_packet data_packet;
  int packet_nr = 0;
  transmitter_packets(fd_file, &start_packet, &end_packet, &data_packet, file_to_send);
  application->file_size = atoi(start_packet.size.value);

  // Fragments of file to send
  unsigned char fragment[FRAG_SIZE];
  unsigned char buffer[MAX_DATA_SIZE];
  int numbytes, size_packet, n_chars_written;

  // Write information
  message("Started llwrite");

  //Start counting time
  start = times(&t);

  // Send START packet
  memset(buffer, '\0', MAX_DATA_SIZE);
  packet_to_array(&start_packet, buffer);
  
  n_chars_written = llwrite(application->fd_port, buffer, START_SIZE);
  LTZ_RET(n_chars_written)

  // Read fragments and send them one by one
  memset(buffer, '\0', MAX_DATA_SIZE);

  while ((numbytes = read(fd_file, fragment, FRAG_SIZE)) != 0) {
    LTZ_RET(numbytes)

    // Send DATA packets
    data_packet.sequence_number = (data_packet.sequence_number + 1) % 256;
    data_packet.nr_bytes2 = numbytes / 256;
    data_packet.nr_bytes1 = numbytes % 256;
    memcpy(data_packet.data, fragment, numbytes);
    packet_to_array(&data_packet, buffer);

    message_packet(packet_nr);
    packet_nr++;
    n_chars_written = llwrite(application->fd_port, buffer, DATA_SIZE);
    LTZ_RET(n_chars_written)
    memset(buffer, '\0', MAX_DATA_SIZE);
  }
  close(fd_file);

  // Send END packet
  packet_to_array(&end_packet, buffer);
  n_chars_written = llwrite(application->fd_port, buffer, END_SIZE);
  LTZ_RET(n_chars_written)
}

int receiver(appLayer *application) {
  // RECEIVER

  // Fragments of file to read
  ctrl_packet start_packet, end_packet;
  data_packet data_packet;
  unsigned char read_buffer[MAX_DATA_SIZE];
  int n_chars_read, size, packet_nr = 0, fd_file;

  receiver_packets(&start_packet, &end_packet, &data_packet);

  // Receive information
  message("Started llread");

  // Read START packet
  memset(read_buffer, '\0', MAX_DATA_SIZE);
  n_chars_read = llread(application->fd_port, read_buffer);
  LTZ_RET(n_chars_read)
  array_to_packet(&start_packet, read_buffer);

  // Create file
  fd_file = open(start_packet.name.value,
                 O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0664);
  LTZ_RET(fd_file)

  // Read fragments
  memset(read_buffer, '\0', MAX_DATA_SIZE);
  message_packet(packet_nr);

  //Start counting time
  start = times(&t);

  while ((n_chars_read = llread(application->fd_port, read_buffer)) != 0) {
    LTZ_RET(n_chars_read);
    if (read_buffer[0] != 3) {
      if (n_chars_read != REJECT_DATA)
        packet_nr++;

      array_to_packet(&data_packet, read_buffer);
      write(fd_file, data_packet.data, data_packet.nr_bytes2 * 256 + data_packet.nr_bytes1);
    } else {
      array_to_packet(&end_packet, read_buffer);
      break;
    }
    memset(read_buffer, '\0', MAX_DATA_SIZE);

    application->file_size = atoi(start_packet.size.value);

    message_packet(packet_nr);
  }

  close(fd_file);
  return 0;
}
