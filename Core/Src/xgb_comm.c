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
  HAL_UART_Transmit(&huart1, (uint8_t*)p_frame, lenght, 1000);

  return 0;
}

static int8_t xgb_prepare_read_frame(u_frame *frame,
                                     const send_frame frame_data) {
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

  // device lenght
  temp_frame.ind_read_frame.device_lenght[0] =
      (frame_data.device_lenght / 16) + '0';
  temp_frame.ind_read_frame.device_lenght[1] =
      (frame_data.device_lenght % 16) + '0';

  // copy device name
  strcpy((char *restrict)temp_frame.ind_read_frame.device_name,
         (const char *)frame_data.p_device_name);

  temp_frame.ind_read_frame.tail_eot = 4;  // EOT

  // trimm message
  uint8_t j = 0;
  for (uint8_t i = 0; i < MAX_FRAME_SIZE; i++) {
    if (temp_frame.frame_bytes[i] != 0) {
      frame->frame_bytes[j] = temp_frame.frame_bytes[i];

      if (temp_frame.frame_bytes[i] == 4)  // if its EOT
      {
        temp_frame.frame_bytes[i + 1] = 0;  // NULL
        return 0;
      }

      j++;
    }
  }

  return -1;
}

uint8_t xgb_read_data(const send_frame frame_data, uint8_t *p_data) {
  u_frame frame;

  xgb_prepare_read_frame(&frame, frame_data);

  xgb_send_frame((uint8_t*)&frame, (const uint8_t)strlen((char*)frame.frame_bytes));

  return 0;
}
