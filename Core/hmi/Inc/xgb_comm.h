/*
 * xgb_comm.h
 *
 *  Created on: Apr 5, 2022
 *      Author: ROJEK
 */

#ifndef INC_XGB_COMM_H_
#define INC_XGB_COMM_H_

#include "assert.h"
#include "stdint.h"

#define MAX_FRAME_SIZE 256
#define MAX_DATA_SIZE 16

#define STATION_NUMBER 1

/*
 * FRAME FORMAT:
 *
 * Request frame (external communication device?XGB):
 * Header(ENQ)|Station number|Command|Command type|Data area|Tail(EOT)|Frame
 * check(BCC)
 *
 * ACK response frame (XGB?external communication device, when receiving data
 * normally): Header(ACK)|Station number|Command|Command type|Data area or
 * NULL|Tail(ETX)|Frame check(BCC)
 *
 * NAK response frame (XGB?external communication device when receiving data
 * abnormally): Header(NAK)|Station number|Command|Command type|Error code
 * (ASCII 4 Byte)|Tail(ETX)|Frame check(BCC)
 */

/*
 * CONTROL CODES
 * XGB_CC_ENQ - Request frame initial code
 * XGB_CC_ACK - ACK response frame initial code
 * XGB_CC_NAK - NAK response frame initial code
 * XGB_CC_EOT - Request frame ending ASCII code
 * XGB_CC_ETX - Response frame ending ASCII code
 */
#define XGB_CC_ENQ 0x05U  // ENQ
#define XGB_CC_ACK 0x06U  // ACK
#define XGB_CC_NAK 0x15U  // NAK
#define XGB_CC_EOT 0x04U  // EOT
#define XGB_CC_ETX 0x03U  // ETX

typedef enum xgb_data_size_marking {
  XGB_DATA_SIZE_BIT = 'X',
  XGB_DATA_SIZE_BYTE = 'B',
  XGB_DATA_SIZE_WORD = 'W',
  XGB_DATA_SIZE_DWORD = 'D',
  XGB_DATA_SIZE_LWORD = 'L'
} xgb_data_size_marking_t;

typedef enum xgb_device_type {
  XGB_DEV_TYPE_P = 'P',
  XGB_DEV_TYPE_M = 'M',
  XGB_DEV_TYPE_K = 'K',
  XGB_DEV_TYPE_F = 'F',
  XGB_DEV_TYPE_T = 'T',
  XGB_DEV_TYPE_C = 'C',
  XGB_DEV_TYPE_L = 'L',
  XGB_DEV_TYPE_N = 'N',
  XGB_DEV_TYPE_D = 'D',
  XGB_DEV_TYPE_U = 'U',
  XGB_DEV_TYPE_Z = 'Z',
  XGB_DEV_TYPE_R = 'R'
} xgb_device_type_t;

typedef enum xgb_comm_error {
  XGB_OK = 0,
  XGB_ERR_TRANSMIT_TIMEOUT = -1,
  XGB_ERR_EOT_MISSING = -2

}xgb_comm_err_t;

/*
 * Parameters required for individual read command frame
 */
struct data_ind_read_frame {
  uint8_t station_number;
  uint8_t no_of_blocks;
  xgb_data_size_marking_t data_size;
  xgb_device_type_t device_type;
  const char *p_device_address;
};

/*
 * Parameters required for individual write command frame
 */
struct data_ind_write_frame {
  uint8_t station_number;
  uint8_t no_of_blocks;
  xgb_data_size_marking_t data_size;
  xgb_device_type_t device_type;
  const char *p_device_address;
  uint8_t *p_data_buffer;
};

/*
 * Parameters required for continuous read command frame
 */
struct data_cont_read_frame {
  uint8_t station_number;
  xgb_data_size_marking_t data_size;
  xgb_device_type_t device_type;
  uint8_t no_of_data;
  const char *p_device_address;
};

/*
 * Parameters required for continuous write command frame
 */
struct data_cont_write_frame {
  uint8_t station_number;
  xgb_data_size_marking_t data_size;
  xgb_device_type_t device_type;
  uint8_t no_of_data;
  const char *p_device_address;
  uint8_t *p_data_buffer;
};

/*
 * To not miss any parameters during command send, structs are created
 */
typedef union cmd_frame_data {
  struct data_ind_read_frame ind_read;
  struct data_ind_write_frame ind_write;
  struct data_cont_read_frame cont_read;
  struct data_cont_write_frame cont_write;
} cmd_frame_data;

typedef uint8_t station_number_t[2];
typedef uint8_t command_t;
typedef uint8_t command_type_t[2];
typedef uint8_t no_blocks_t[2];
typedef uint8_t device_lenght_t[2];
typedef uint8_t no_data_t[2];
typedef uint8_t device_name_t[16];
typedef uint8_t header_tail_t;

__attribute__((packed)) struct respond_ack_frame {
  header_tail_t header_ack;
  station_number_t station_number;
  command_t command;
  command_type_t command_type;
  no_blocks_t no_blocks;
  no_data_t no_data;
  uint8_t data[245];
  header_tail_t tail_eot;
};

__attribute__((packed)) struct respond_nak_frame {
  header_tail_t header_nak;
  station_number_t station_number;
  command_t command;
  command_type_t command_type;
  no_blocks_t no_blocks;
  uint8_t error_code[4];
};

__attribute__((packed)) struct cmd_ind_read_frame {
  header_tail_t header_enq;
  station_number_t station_number;
  command_t command;
  command_type_t command_type;
  no_blocks_t no_blocks;
  device_lenght_t device_lenght;
  device_name_t device_name;
  header_tail_t tail_eot;
};

__attribute__((packed)) struct cmd_ind_write_frame {
  header_tail_t header_enq;
  station_number_t station_number;
  command_t command;
  command_type_t command_type;
  no_blocks_t no_blocks;
  device_lenght_t device_lenght;
  device_name_t device_name;
  uint8_t data[229];
  header_tail_t tail_eot;
};

__attribute__((packed)) struct cmd_cont_read_frame {
  header_tail_t header_enq;
  station_number_t station_number;
  command_t command;
  command_type_t command_type;
  device_lenght_t device_lenght;
  device_name_t device_name;
  no_data_t no_data;
  header_tail_t tail_eot;
};

__attribute__((packed)) struct cmd_cont_write_frame {
  header_tail_t header_enq;
  station_number_t station_number;
  command_t command;
  command_type_t command_type;
  device_lenght_t device_lenght;
  device_name_t device_name;
  no_data_t no_data;
  uint8_t data[229];
  header_tail_t tail_eot;
};

static_assert(sizeof(struct respond_ack_frame) <= MAX_FRAME_SIZE,
              "frame struct size exceeded");
static_assert(sizeof(struct respond_nak_frame) <= MAX_FRAME_SIZE,
              "frame struct size exceeded");
static_assert(sizeof(struct cmd_ind_read_frame) <= MAX_FRAME_SIZE,
              "frame struct size exceeded");
static_assert(sizeof(struct cmd_ind_write_frame) <= MAX_FRAME_SIZE,
              "frame struct size exceeded");
static_assert(sizeof(struct cmd_cont_read_frame) <= MAX_FRAME_SIZE,
              "frame struct size exceeded");
static_assert(sizeof(struct cmd_cont_write_frame) <= MAX_FRAME_SIZE,
              "frame struct size exceeded");


typedef union frame {
  uint8_t frame_bytes[MAX_FRAME_SIZE];
  struct respond_ack_frame ack_frame;
  struct respond_nak_frame nak_frame;
  struct cmd_ind_read_frame ind_read_frame;
  struct cmd_ind_write_frame ind_write_frame;
  struct cmd_cont_read_frame cont_read_frame;
  struct cmd_cont_write_frame cont_write_frame;
} u_frame;


xgb_comm_err_t xgb_read_single_device(const xgb_device_type_t type,
                                      const xgb_data_size_marking_t size_mark,
                                      const char *address);

#endif /* INC_XGB_COMM_H_ */
