/*
 * xgb_comm.c
 *
 *  Created on: 11 kwi 2022
 *      Author: ROJEK
 */

#include "xgb_comm.h"

#include "main.h"
#include "stdio.h"
#include "string.h"

extern UART_HandleTypeDef huart1;

static uint8_t xgb_send_frame(const uint8_t *p_frame, const uint8_t lenght) {
  if ((HAL_UART_Transmit(&huart1, (uint8_t *)p_frame, lenght, 1000) !=
       HAL_OK)) {
    return -1;
  }

  return 0;
}

static int8_t xgb_prepare_individual_read_frame(
    u_frame *frame, const command_frame frame_data) {
  // prepare message - fill union with 0s
  u_frame temp_frame = {0};

  // header
  temp_frame.ind_read_frame.header_enq = 5;  // ENQ

  // station number
  temp_frame.ind_read_frame.station_no[0] = (frame_data.station_no / 16) + '0';
  temp_frame.ind_read_frame.station_no[1] = (frame_data.station_no % 16) + '0';

  // command
  temp_frame.ind_read_frame.command = 'R';

  // command type
  temp_frame.ind_read_frame.command_type[0] = 'S';
  temp_frame.ind_read_frame.command_type[1] = 'S';

  // no blocks
  temp_frame.ind_read_frame.no_blocks[0] = (frame_data.no_of_blocks / 16) + '0';
  temp_frame.ind_read_frame.no_blocks[1] = (frame_data.no_of_blocks % 16) + '0';

  // device lenght %MW <- this is 3 chars and then we add lenght of address
  // %MW100 = 3 + strlen("100") = 6
  temp_frame.ind_read_frame.device_lenght[0] =
      ((3 + strlen(frame_data.p_device_address)) / 16) + '0';
  temp_frame.ind_read_frame.device_lenght[1] =
      ((3 + strlen(frame_data.p_device_address)) % 16) + '0';

  // prepare device name
  temp_frame.ind_read_frame.device_name[0] = '%';
  temp_frame.ind_read_frame.device_name[1] =
      frame_data.device_type;  // device memory group (P,M,L etc.)
  temp_frame.ind_read_frame.device_name[2] = frame_data.data_size;
  strcpy((char *restrict)(&temp_frame.ind_read_frame.device_name[3]),
         (const char *)frame_data.p_device_address);

  temp_frame.ind_read_frame.tail_eot = 4;  // EOT

  // trimm message
  uint8_t j = 0;
  for (uint8_t i = 0; i < MAX_FRAME_SIZE; i++) {
    if (temp_frame.frame_bytes[i] != 0) {
      frame->frame_bytes[j] = temp_frame.frame_bytes[i];

      if (temp_frame.frame_bytes[i] == 4)  // if its EOT
      {
        frame->frame_bytes[j + 1] = 0;  // NULL
        return 0;
      }

      j++;
    }
  }

  return -1;
}

static int8_t xgb_prepare_individual_write_frame(
    u_frame *frame, const command_frame frame_data) {
  // prepare message - fill union with 0s
  u_frame temp_frame = {0};

  // header
  temp_frame.ind_write_frame.header_enq = 5;  // ENQ

  // station number
  temp_frame.ind_write_frame.station_no[0] = (frame_data.station_no / 16) + '0';
  temp_frame.ind_write_frame.station_no[1] = (frame_data.station_no % 16) + '0';

  // command
  temp_frame.ind_write_frame.command = 'W';

  // command type
  temp_frame.ind_write_frame.command_type[0] = 'S';
  temp_frame.ind_write_frame.command_type[1] = 'S';

  // no blocks
  temp_frame.ind_write_frame.no_blocks[0] =
      (frame_data.no_of_blocks / 16) + '0';
  temp_frame.ind_write_frame.no_blocks[1] =
      (frame_data.no_of_blocks % 16) + '0';

  // device lenght %MW <- this is 3 chars and then we add lenght of address
  // %MW100 = 3 + strlen("100") = 6
  temp_frame.ind_write_frame.device_lenght[0] =
      ((3 + strlen(frame_data.p_device_address)) / 16) + '0';
  temp_frame.ind_write_frame.device_lenght[1] =
      ((3 + strlen(frame_data.p_device_address)) % 16) + '0';

  // prepare device name
  temp_frame.ind_write_frame.device_name[0] = '%';
  temp_frame.ind_write_frame.device_name[1] =
      frame_data.device_type;  // device memory group (P,M,L etc.)
  temp_frame.ind_write_frame.device_name[2] = frame_data.data_size;
  strcpy((char *restrict)(&temp_frame.ind_read_frame.device_name[3]),
         (const char *)frame_data.p_device_address);

  // prepare frame data
  uint8_t no_bytes_to_copy = frame_data.no_of_blocks *
                             (data_marking_to_size(frame_data.data_size) * 2);
  memcpy(temp_frame.ind_write_frame.data, frame_data.p_data_buffer,
         no_bytes_to_copy);

  for (uint8_t i = 0; i < no_bytes_to_copy; i++) {
    temp_frame.ind_write_frame.data[i] += '0';
  }

  temp_frame.ind_write_frame.tail_eot = 4;  // EOT

  // trimm message
  uint8_t j = 0;
  for (uint8_t i = 0; i < MAX_FRAME_SIZE; i++) {
    if (temp_frame.frame_bytes[i] != 0) {
      frame->frame_bytes[j] = temp_frame.frame_bytes[i];

      if (temp_frame.frame_bytes[i] == 4)  // if its EOT
      {
        frame->frame_bytes[j + 1] = 0;  // NULL
        return 0;
      }

      j++;
    }
  }

  return -1;
}

static int8_t xgb_prepare_continuous_read_frame(
    u_frame *frame, const command_frame frame_data) {
  // prepare message - fill union with 0s
  u_frame temp_frame = {0};

  // header
  temp_frame.cont_read_frame.header_enq = 5;  // ENQ

  // station number
  temp_frame.ind_read_frame.station_no[0] = (frame_data.station_no / 16) + '0';
  temp_frame.ind_read_frame.station_no[1] = (frame_data.station_no % 16) + '0';

  // command
  temp_frame.ind_read_frame.command = 'R';

  // command type
  temp_frame.ind_read_frame.command_type[0] = 'S';
  temp_frame.ind_read_frame.command_type[1] = 'S';

  // no blocks
  temp_frame.ind_read_frame.no_blocks[0] = (frame_data.no_of_blocks / 16) + '0';
  temp_frame.ind_read_frame.no_blocks[1] = (frame_data.no_of_blocks % 16) + '0';

  // device lenght %MW <- this is 3 chars and then we add lenght of address
  // %MW100 = 3 + strlen("100") = 6
  temp_frame.ind_read_frame.device_lenght[0] =
      ((3 + strlen(frame_data.p_device_address)) / 16) + '0';
  temp_frame.ind_read_frame.device_lenght[1] =
      ((3 + strlen(frame_data.p_device_address)) % 16) + '0';

  // prepare device name
  temp_frame.ind_read_frame.device_name[0] = '%';
  temp_frame.ind_read_frame.device_name[1] =
      frame_data.device_type;  // device memory group (P,M,L etc.)
  temp_frame.ind_read_frame.device_name[2] = frame_data.data_size;
  strcpy((char *restrict)(&temp_frame.ind_read_frame.device_name[3]),
         (const char *)frame_data.p_device_address);

  temp_frame.ind_read_frame.tail_eot = 4;  // EOT

  // trimm message
  uint8_t j = 0;
  for (uint8_t i = 0; i < MAX_FRAME_SIZE; i++) {
    if (temp_frame.frame_bytes[i] != 0) {
      frame->frame_bytes[j] = temp_frame.frame_bytes[i];

      if (temp_frame.frame_bytes[i] == 4)  // if its EOT
      {
        frame->frame_bytes[j + 1] = 0;  // NULL
        return 0;
      }

      j++;
    }
  }

  return -1;
}

uint8_t xgb_send_individual_read_cmd(const command_frame frame_data) {
  u_frame frame;

  if (xgb_prepare_individual_read_frame(&frame, frame_data) != 0) {
    return -1;
  }

  if (xgb_send_frame((uint8_t *)&frame,
                     (const uint8_t)strlen((char *)frame.frame_bytes)) != 0) {
    return -2;
  }

  return 0;
}

uint8_t xgb_send_individual_write_cmd(const command_frame frame_data) {
  u_frame frame;

  if (xgb_prepare_individual_write_frame(&frame, frame_data) != 0) {
    return -1;
  }

  if (xgb_send_frame((uint8_t *)&frame,
                     (const uint8_t)strlen((char *)frame.frame_bytes)) != 0) {
    return -2;
  }
  return 0;
}

uint8_t xgb_send_continious_read_cmd(const command_frame frame_data) {
  u_frame frame;

  if (xgb_prepare_individual_read_frame(&frame, frame_data) != 0) {
    return -1;
  }

  if (xgb_send_frame((uint8_t *)&frame,
                     (const uint8_t)strlen((char *)frame.frame_bytes)) != 0) {
    return -2;
  }

  return 0;
}
