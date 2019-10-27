#include "packet.h"

void make_packets(int fd_file, ctrl_packet *start_packet, ctrl_packet *end_packet, data_packet *data_packet) {
  struct stat file_stat;
  if (fstat(fd_file, &file_stat) < 0) {
    message("Error reading file. Exitting.");
    exit(2);
  }

  start_packet->control = 2;
  start_packet->size.type = 0;
  start_packet->size.length = sizeof(int) / sizeof(char);
  start_packet->size.value = malloc(start_packet->size.length);
  sprintf(start_packet->size.value, "%ld", file_stat.st_size);

  start_packet->name.type = 1;
  start_packet->name.length = sizeof(char) * strlen(basename(FILE_TO_SEND));
  start_packet->name.value = malloc(start_packet->name.length);
  sprintf(start_packet->size.value, "%s", basename(FILE_TO_SEND));

  data_packet->control = 1;
  data_packet->sequence_number = 255;
  data_packet->nr_bytes2 = FRAG_SIZE / 256;
  data_packet->nr_bytes1 = FRAG_SIZE % 256;
  data_packet->data = malloc(FRAG_SIZE);

  end_packet->control = 3;
  end_packet->size.type = 0;
  end_packet->size.length = sizeof(int) / sizeof(char);
  end_packet->size.value = malloc(end_packet->size.length);
  sprintf(end_packet->size.value, "%ld", file_stat.st_size);

  end_packet->name.type = 1;
  end_packet->name.length = sizeof(char) * strlen(basename(FILE_TO_SEND));
  end_packet->name.value = malloc(end_packet->name.length);
  sprintf(end_packet->size.value, "%s", basename(FILE_TO_SEND));
}

void packet_to_array(void *packet_void_ptr, char *buffer) {
  data_packet *data_packet_ptr = (data_packet *)packet_void_ptr;
  ctrl_packet *ctrl_packet_ptr = (ctrl_packet *)packet_void_ptr;

  switch (data_packet_ptr->control) {
  // DATA PACKET
  case 1:
    buffer[0] = data_packet_ptr->control;
    buffer[1] = data_packet_ptr->sequence_number;
    buffer[2] = data_packet_ptr->nr_bytes2;
    buffer[3] = data_packet_ptr->nr_bytes1;
    strcpy((buffer + 4), data_packet_ptr->data);
    break;
  // START PACKET
  case 2:
    buffer[0] = ctrl_packet_ptr->control;
    buffer[1] = ctrl_packet_ptr->size.type;
    buffer[2] = ctrl_packet_ptr->size.length;
    strcpy((buffer + 3), ctrl_packet_ptr->size.value);
    buffer[3 + ctrl_packet_ptr->size.length] = ctrl_packet_ptr->name.type;
    buffer[4 + ctrl_packet_ptr->size.length] = ctrl_packet_ptr->name.length;
    strcpy((buffer + 5 + ctrl_packet_ptr->size.length), ctrl_packet_ptr->name.value);
    break;
  // END PACKET
  case 3:
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
}

void array_to_packet(void *packet_void_ptr, char *buffer) {
  data_packet *data_packet_ptr = (data_packet *)packet_void_ptr;
  ctrl_packet *ctrl_packet_ptr = (ctrl_packet *)packet_void_ptr;
  
  switch (buffer[0]) {
  // DATA
  case 1:
    data_packet_ptr->control = buffer[0];
    data_packet_ptr->sequence_number = buffer[1];
    data_packet_ptr->nr_bytes2 = buffer[2];
    data_packet_ptr->nr_bytes1 = buffer[3];
    strcpy(data_packet_ptr->data, (buffer + 4));
    break;
  // START
  case 2:
    ctrl_packet_ptr->control = buffer[0];
    ctrl_packet_ptr->size.type = buffer[1];
    ctrl_packet_ptr->size.length = buffer[2];
    ctrl_packet_ptr->size.value = malloc(buffer[2]);
    if (ctrl_packet_ptr->size.value == NULL) {
      message("Failed to allocate space. SIZE VALUE");
      exit(3);
    }
    strcpy(ctrl_packet_ptr->size.value, (buffer + 3));

    ctrl_packet_ptr->name.type = buffer[3 + buffer[2]];
    ctrl_packet_ptr->name.length = buffer[4 + buffer[2]];
    ctrl_packet_ptr->name.value = malloc(buffer[4 + buffer[2]]);
    if (ctrl_packet_ptr->name.value == NULL) {
      message("Failed to allocate space. NAME VALUE");
      exit(3);
    }
    
    strcpy(ctrl_packet_ptr->size.value, (buffer + 5 + buffer[2]));
    break;
  // END
  case 3:
    ctrl_packet_ptr->control = buffer[0];
    ctrl_packet_ptr->size.type = buffer[1];
    ctrl_packet_ptr->size.length = buffer[2];
    ctrl_packet_ptr->size.value = malloc(buffer[2]);
    if (ctrl_packet_ptr->size.value == NULL) {
      message("Failed to allocate space. SIZE VALUE");
      exit(3);
    }
    strcpy(ctrl_packet_ptr->size.value, (buffer + 3));

    ctrl_packet_ptr->name.type = buffer[3 + buffer[2]];
    ctrl_packet_ptr->name.length = buffer[4 + buffer[2]];
    ctrl_packet_ptr->name.value = malloc(buffer[4 + buffer[2]]);
    if (ctrl_packet_ptr->name.value == NULL) {
      message("Failed to allocate space. SIZE VALUE");
      exit(3);
    }
    strcpy(ctrl_packet_ptr->size.value, (buffer + 5 + buffer[2]));
    break;
  default:
    break;
  }
}
