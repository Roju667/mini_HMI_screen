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

/*
 * Basic send frame function, uart transmit function
 */
static uint8_t send_frame(const uint8_t *p_frame, const uint8_t lenght)
{
  if ((HAL_UART_Transmit(&huart1, (uint8_t *)p_frame, lenght, 1000) != HAL_OK))
    {
      return XGB_ERR_TRANSMIT_TIMEOUT;
    }

  return XGB_OK;
}

/*
 * Delete all the empty spaces between parts of frame and add NULL at the end
 */
static uint8_t prepare_frame(const u_frame *raw_frame, u_frame *ready_frame)
{
  uint8_t j = 0;

  // go trough array and if cell is not empty
  // rewrite it to ready frame
  for (uint8_t i = 0; i < MAX_FRAME_SIZE; i++)
    {
      if (raw_frame->frame_bytes[i] != 0)
        {
          ready_frame->frame_bytes[j] = raw_frame->frame_bytes[i];

          if (raw_frame->frame_bytes[i] == XGB_CC_EOT) // if its EOT
            {
              // finish the message with NULL to create a string
              ready_frame->frame_bytes[j + 1] = 0; // NULL
              return XGB_OK;
            }

          j++;
        }
    }
  return XGB_ERR_EOT_MISSING;
}

/*
 * Prepare frame - request of individual read
 */
static int8_t prepare_individual_read_frame(u_frame *frame,
                                            const cmd_frame_data *p_frame_data)
{
  // prepare message - fill union with 0s
  u_frame temp_frame = {0};

  // header
  temp_frame.ind_read_frame.header_enq = XGB_CC_ENQ;

  // station number
  temp_frame.ind_read_frame.station_number[0] =
      (p_frame_data->ind_read.station_number / 16) + '0';
  temp_frame.ind_read_frame.station_number[1] =
      (p_frame_data->ind_read.station_number % 16) + '0';

  // command
  temp_frame.ind_read_frame.command = 'R';

  // command type
  temp_frame.ind_read_frame.command_type[0] = 'S';
  temp_frame.ind_read_frame.command_type[1] = 'S';

  // no blocks
  temp_frame.ind_read_frame.no_blocks[0] =
      (p_frame_data->ind_read.no_of_blocks / 16) + '0';
  temp_frame.ind_read_frame.no_blocks[1] =
      (p_frame_data->ind_read.no_of_blocks % 16) + '0';

  // device lenght %MW <- this is 3 chars and then we add lenght of address
  // %MW100 = 3 + strlen("100") = 6
  temp_frame.ind_read_frame.device_lenght[0] =
      ((3 + strlen(p_frame_data->ind_read.p_device_address)) / 16) + '0';
  temp_frame.ind_read_frame.device_lenght[1] =
      ((3 + strlen(p_frame_data->ind_read.p_device_address)) % 16) + '0';

  // prepare device name
  temp_frame.ind_read_frame.device_name[0] = '%';
  temp_frame.ind_read_frame.device_name[1] =
      p_frame_data->ind_read.device_type; // device memory group (P,M,L etc.)
  temp_frame.ind_read_frame.device_name[2] = p_frame_data->ind_read.data_size;
  strcpy((char *restrict)(&temp_frame.ind_read_frame.device_name[3]),
         (const char *)p_frame_data->ind_read.p_device_address);

  temp_frame.ind_read_frame.tail_eot = XGB_CC_EOT;

  // prepare message
  return prepare_frame(&temp_frame, frame);
}

/*
 * Prepare frame - request of individual write
 */
static int8_t prepare_individual_write_frame(u_frame *frame,
                                             const cmd_frame_data *p_frame_data)
{
  // prepare message - fill union with 0s
  u_frame temp_frame = {0};

  // header
  temp_frame.ind_write_frame.header_enq = XGB_CC_ENQ;

  // station number
  temp_frame.ind_write_frame.station_number[0] =
      (p_frame_data->ind_write.station_number / 16) + '0';
  temp_frame.ind_write_frame.station_number[1] =
      (p_frame_data->ind_write.station_number % 16) + '0';

  // command
  temp_frame.ind_write_frame.command = 'W';

  // command type
  temp_frame.ind_write_frame.command_type[0] = 'S';
  temp_frame.ind_write_frame.command_type[1] = 'S';

  // no blocks
  temp_frame.ind_write_frame.no_blocks[0] =
      (p_frame_data->ind_write.no_of_blocks / 16) + '0';
  temp_frame.ind_write_frame.no_blocks[1] =
      (p_frame_data->ind_write.no_of_blocks % 16) + '0';

  // device lenght %MW <- this is 3 chars and then we add lenght of address
  // %MW100 = 3 + strlen("100") = 6
  temp_frame.ind_write_frame.device_lenght[0] =
      ((3 + strlen(p_frame_data->ind_write.p_device_address)) / 16) + '0';
  temp_frame.ind_write_frame.device_lenght[1] =
      ((3 + strlen(p_frame_data->ind_write.p_device_address)) % 16) + '0';

  // prepare device name
  temp_frame.ind_write_frame.device_name[0] = '%';
  temp_frame.ind_write_frame.device_name[1] =
      p_frame_data->ind_write.device_type; // device memory group (P,M,L etc.)
  temp_frame.ind_write_frame.device_name[2] = p_frame_data->ind_write.data_size;
  strcpy((char *restrict)(&temp_frame.ind_read_frame.device_name[3]),
         (const char *)p_frame_data->ind_write.p_device_address);

  // prepare frame data
  uint8_t no_bytes_to_copy =
      p_frame_data->ind_write.no_of_blocks *
      (data_marking_to_size(p_frame_data->ind_write.data_size) * 2);
  memcpy(temp_frame.ind_write_frame.data, p_frame_data->ind_write.p_data_buffer,
         no_bytes_to_copy);

  for (uint8_t i = 0; i < no_bytes_to_copy; i++)
    {
      temp_frame.ind_write_frame.data[i] += '0';
    }

  temp_frame.ind_write_frame.tail_eot = XGB_CC_EOT; // EOT

  return prepare_frame(&temp_frame, frame);
}

/*
 * Prepare frame - request of continuous read
 */
static int8_t prepare_continuous_read_frame(u_frame *frame,
                                            const cmd_frame_data *p_frame_data)
{
  // prepare message - fill union with 0s
  u_frame temp_frame = {0};

  // header
  temp_frame.cont_read_frame.header_enq = XGB_CC_ENQ; // ENQ

  // station number
  temp_frame.cont_read_frame.station_number[0] =
      (p_frame_data->cont_read.station_number / 16) + '0';
  temp_frame.cont_read_frame.station_number[1] =
      (p_frame_data->cont_read.station_number % 16) + '0';

  // command
  temp_frame.cont_read_frame.command = 'R';

  // command type
  temp_frame.cont_read_frame.command_type[0] = 'S';
  temp_frame.cont_read_frame.command_type[1] = 'B';

  // device lenght %MW <- this is 3 chars and then we add lenght of address
  // %MW100 = 3 + strlen("100") = 6
  temp_frame.cont_read_frame.device_lenght[0] =
      ((3 + strlen(p_frame_data->cont_read.p_device_address)) / 16) + '0';
  temp_frame.cont_read_frame.device_lenght[1] =
      ((3 + strlen(p_frame_data->cont_read.p_device_address)) % 16) + '0';

  // prepare device name
  temp_frame.cont_read_frame.device_name[0] = '%';
  temp_frame.cont_read_frame.device_name[1] =
      p_frame_data->cont_read.device_type; // device memory group (P,M,L etc.)
  temp_frame.cont_read_frame.device_name[2] = p_frame_data->cont_read.data_size;
  strcpy((char *restrict)(&temp_frame.cont_read_frame.device_name[3]),
         (const char *)p_frame_data->cont_read.p_device_address);

  // no of data
  temp_frame.cont_read_frame.no_data[0] =
      (p_frame_data->cont_read.no_of_data / 16) + '0';
  temp_frame.cont_read_frame.no_data[1] =
      (p_frame_data->cont_read.no_of_data % 16) + '0';

  temp_frame.cont_read_frame.tail_eot = XGB_CC_EOT; // EOT

  // trimm message
  return prepare_frame(&temp_frame, frame);
}

/*
 * Prepare frame - request of continuous write
 */
static int8_t prepare_continuous_write_frame(u_frame *frame,
                                             const cmd_frame_data *p_frame_data)
{
  // prepare message - fill union with 0s
  u_frame temp_frame = {0};

  // header
  temp_frame.cont_write_frame.header_enq = XGB_CC_ENQ; // ENQ

  // station number
  temp_frame.cont_write_frame.station_number[0] =
      (p_frame_data->cont_write.station_number / 16) + '0';
  temp_frame.cont_write_frame.station_number[1] =
      (p_frame_data->cont_write.station_number % 16) + '0';

  // command
  temp_frame.cont_write_frame.command = 'W';

  // command type
  temp_frame.cont_write_frame.command_type[0] = 'S';
  temp_frame.cont_write_frame.command_type[1] = 'B';

  // device lenght %MW <- this is 3 chars and then we add lenght of address
  // %MW100 = 3 + strlen("100") = 6
  temp_frame.cont_write_frame.device_lenght[0] =
      ((3 + strlen(p_frame_data->cont_write.p_device_address)) / 16) + '0';
  temp_frame.cont_write_frame.device_lenght[1] =
      ((3 + strlen(p_frame_data->cont_write.p_device_address)) % 16) + '0';

  // prepare device name
  temp_frame.cont_write_frame.device_name[0] = '%';
  temp_frame.cont_write_frame.device_name[1] =
      p_frame_data->cont_write.device_type; // device memory group (P,M,L etc.)
  temp_frame.cont_write_frame.device_name[2] =
      p_frame_data->cont_write.data_size;
  strcpy((char *restrict)(&temp_frame.cont_write_frame.device_name[3]),
         (const char *)p_frame_data->cont_write.p_device_address);

  // no of data
  temp_frame.cont_write_frame.no_data[0] =
      (p_frame_data->cont_write.no_of_data / 16) + '0';
  temp_frame.cont_write_frame.no_data[1] =
      (p_frame_data->cont_write.no_of_data % 16) + '0';

  // prepare frame data
  uint8_t no_bytes_to_copy =
      p_frame_data->cont_write.no_of_data *
      (data_marking_to_size(p_frame_data->cont_write.data_size) * 2);

  memcpy(temp_frame.cont_write_frame.data,
         p_frame_data->cont_write.p_data_buffer, no_bytes_to_copy);

  for (uint8_t i = 0; i < no_bytes_to_copy; i++)
    {
      temp_frame.cont_write_frame.data[i] += '0';
    }

  temp_frame.cont_write_frame.tail_eot = XGB_CC_EOT; // EOT

  // trimm message
  return prepare_frame(&temp_frame, frame);

  return XGB_ERR_EOT_MISSING;
}

/*
 * Send request of individual read
 */
uint8_t xgb_send_individual_read_cmd(const cmd_frame_data *p_frame_data)
{
  u_frame frame;

  if (prepare_individual_read_frame(&frame, p_frame_data) != XGB_OK)
    {
      return XGB_ERR_EOT_MISSING;
    }

  if (send_frame((uint8_t *)&frame,
                 (const uint8_t)strlen((char *)frame.frame_bytes)) != XGB_OK)
    {
      return XGB_ERR_TRANSMIT_TIMEOUT;
    }

  return XGB_OK;
}

/*
 * Send request of individual write
 */
uint8_t xgb_send_individual_write_cmd(const cmd_frame_data *p_frame_data)
{
  u_frame frame;

  if (prepare_individual_write_frame(&frame, p_frame_data) != XGB_OK)
    {
      return XGB_ERR_EOT_MISSING;
    }

  if (send_frame((uint8_t *)&frame,
                 (const uint8_t)strlen((char *)frame.frame_bytes)) != XGB_OK)
    {
      return XGB_ERR_TRANSMIT_TIMEOUT;
    }
  return XGB_OK;
}

/*
 * Send request of continuous read
 */
uint8_t xgb_send_continuous_read_cmd(const cmd_frame_data *p_frame_data)
{
  u_frame frame;

  if (prepare_continuous_read_frame(&frame, p_frame_data) != XGB_OK)
    {
      return XGB_ERR_EOT_MISSING;
    }

  if (send_frame((uint8_t *)&frame,
                 (const uint8_t)strlen((char *)frame.frame_bytes)) != XGB_OK)
    {
      return XGB_ERR_TRANSMIT_TIMEOUT;
    }

  return XGB_OK;
}

/*
 * Send request of continuous write
 */
uint8_t xgb_send_continuous_write_cmd(const cmd_frame_data *p_frame_data)
{
  u_frame frame;

  if (prepare_continuous_write_frame(&frame, p_frame_data) != XGB_OK)
    {
      return XGB_ERR_EOT_MISSING;
    }

  if (send_frame((uint8_t *)&frame,
                 (const uint8_t)strlen((char *)frame.frame_bytes)) != XGB_OK)
    {
      return XGB_ERR_TRANSMIT_TIMEOUT;
    }

  return XGB_OK;
}

/*
 *
 */
uint8_t xgb_read_single_device(const xgb_device_type type,
                               const xgb_data_size_marking size_mark,
                               const char *address)
{
  cmd_frame_data frame = {0};

  frame.ind_read.data_size = size_mark;
  frame.ind_read.device_type = type;
  frame.ind_read.no_of_blocks = 1;
  frame.ind_read.p_device_address = address;
  frame.ind_read.station_number = STATION_NUMBER;

  xgb_send_individual_read_cmd(&frame);
}
