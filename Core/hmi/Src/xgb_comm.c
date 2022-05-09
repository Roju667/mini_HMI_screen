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

typedef xgb_comm_err_t (*prep_frame_fun_t)(u_frame *frame,
                                           const cmd_frame_data *p_frame_data);

typedef enum prep_frame
{
  INDIVI_READ = 0,
  CONT_READ = 1,
  INDIVI_WRITE = 2,
  CONT_WRITE = 3
} prep_frame_ID;

typedef struct prep_fun_mapper
{
  prep_frame_ID ID;
  prep_frame_fun_t function;
} prep_fun_mapper_t;

extern UART_HandleTypeDef huart1;

// basic
static xgb_comm_err_t send_frame(const uint8_t *p_frame, uint8_t lenght);
static xgb_comm_err_t prep_frame(const u_frame *raw_frame,
                                 u_frame *ready_frame);

// specific frame prep
static xgb_comm_err_t
prep_indivi_read_frame(u_frame *frame, const cmd_frame_data *p_frame_data);
static xgb_comm_err_t prep_cont_read_frame(u_frame *frame,
                                           const cmd_frame_data *p_frame_data);
static xgb_comm_err_t
prep_indivi_write_frame(u_frame *frame, const cmd_frame_data *p_frame_data);
static xgb_comm_err_t prep_cont_write_frame(u_frame *frame,
                                            const cmd_frame_data *p_frame_data);
// send specific frame
static xgb_comm_err_t send_specific_cmd(const cmd_frame_data *p_frame_data,prep_frame_ID ID);
// utility
static uint8_t data_marking_to_size(xgb_data_size_marking_t data_size);

xgb_comm_err_t xgb_read_single_device(const xgb_device_type_t type,
                                      const xgb_data_size_marking_t size_mark,
                                      const char *address)
{
  cmd_frame_data frame = {0};
  xgb_comm_err_t comm_status = XGB_OK;

  frame.ind_read.data_size = size_mark;
  frame.ind_read.device_type = type;
  frame.ind_read.no_of_blocks = 1;
  frame.ind_read.p_device_address = address;
  frame.ind_read.station_number = STATION_NUMBER;

  comm_status = send_specific_cmd(&frame,INDIVI_READ);

  return comm_status;
}

/*
 * Basic send frame function, uart transmit function
 */
static xgb_comm_err_t send_frame(const uint8_t *p_frame, uint8_t lenght)
{
  xgb_comm_err_t comm_status = XGB_OK;

  if ((HAL_UART_Transmit(&huart1, (uint8_t *)p_frame, lenght, 1000) != HAL_OK))
    {
      comm_status = XGB_ERR_TRANSMIT_TIMEOUT;
    }

  return comm_status;
}

/*
 * Delete all the empty spaces between parts of frame and add NULL at the end
 */
static xgb_comm_err_t prep_frame(const u_frame *raw_frame, u_frame *ready_frame)
{
  uint8_t j = 0;
  xgb_comm_err_t comm_status = XGB_ERR_EOT_MISSING;

  // go trough array and if cell is not empty
  // rewrite it to ready frame
  for (uint8_t i = 0; i < MAX_FRAME_SIZE; i++)
    {
      if (0 != raw_frame->frame_bytes[i])
        {
          ready_frame->frame_bytes[j] = raw_frame->frame_bytes[i];

          if (XGB_CC_EOT == raw_frame->frame_bytes[i]) // if its EOT
            {
              // finish the message with NULL to create a string
              ready_frame->frame_bytes[j + 1] = 0; // NULL
              comm_status = XGB_OK;
            }

          j++;
        }
    }

  return comm_status;
}

/*
 * Prepare frame - request of individual read
 */
static xgb_comm_err_t prep_indivi_read_frame(u_frame *frame,
                                             const cmd_frame_data *p_frame_data)
{
  u_frame temp_frame = {0};
  xgb_comm_err_t comm_status = XGB_OK;

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
  comm_status = prep_frame(&temp_frame, frame);
  return comm_status;
}

/*
 * Prepare frame - request of individual write
 */
static xgb_comm_err_t
prep_indivi_write_frame(u_frame *frame, const cmd_frame_data *p_frame_data)
{
  // prepare message - fill union with 0s
  u_frame temp_frame = {0};
  xgb_comm_err_t comm_status = XGB_OK;
  ;

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

  return prep_frame(&temp_frame, frame);
}

/*
 * Prepare frame - request of continuous read
 */
static xgb_comm_err_t prep_cont_read_frame(u_frame *frame,
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
  return prep_frame(&temp_frame, frame);
}

/*
 * Prepare frame - request of continuous write
 */
static xgb_comm_err_t prep_cont_write_frame(u_frame *frame,
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
  return prep_frame(&temp_frame, frame);

  return XGB_ERR_EOT_MISSING;
}


static xgb_comm_err_t send_specific_cmd(const cmd_frame_data *p_frame_data,prep_frame_ID ID)
{

  const static prep_fun_mapper_t prep_fun_mapper[] = {{INDIVI_READ, prep_indivi_read_frame},
                                  {CONT_READ, prep_cont_read_frame},
                                  {INDIVI_WRITE, prep_indivi_write_frame},
                                  {CONT_WRITE, prep_cont_write_frame}};

  xgb_comm_err_t status = XGB_OK;
  u_frame frame;

  if (XGB_OK != prep_fun_mapper[ID].function(&frame, p_frame_data))
    {
      status = XGB_ERR_EOT_MISSING;
    }else
    {
    	  if (XGB_OK !=
    	      send_frame((uint8_t *)&frame, (uint8_t)strlen((char *)frame.frame_bytes)))
    	    {
    	      status = XGB_ERR_TRANSMIT_TIMEOUT;
    	    }
    }

  return status;
}

// converts marking to amount of bytes
static uint8_t data_marking_to_size(xgb_data_size_marking_t data_size)
{
  switch (data_size)
    {
    case (XGB_DATA_SIZE_BIT):
      {
        return 1;
      }
    case (XGB_DATA_SIZE_BYTE):
      {
        return 1;
      }
    case (XGB_DATA_SIZE_WORD):
      {
        return 2;
      }
    case (XGB_DATA_SIZE_DWORD):
      {
        return 4;
      }
    case (XGB_DATA_SIZE_LWORD):
      {
        return 8;
      }
    default:
      {
        return 0;
      }
    }
}
