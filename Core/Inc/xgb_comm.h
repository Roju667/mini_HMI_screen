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

/*
 * COMMANDS
 * Read write command is a combination of Read/Write hex code with
 * Individual/Continious hexcode
 *
 */
#define XBG_CMD_READ 0x52U             // R
#define XBG_CMD_WRITE 0x57U            // W
#define XBG_CMD_INDIVIDUAL_RW 0x5353U  // SS
#define XBG_CMD_CONTINUOUS_RW 0x5342U  // SB

#define XBG_CMD_REGISTER_MONITORING 0x58U  // X
#define XBG_CMD_EXECUTE_MONITORING 0x59U   // Y

typedef enum xgb_data_size_marking {
  XGB_DATA_SIZE_BIT = 'X',
  XGB_DATA_SIZE_BYTE = 'B',
  XGB_DATA_SIZE_WORD = 'W',
  XGB_DATA_SIZE_DWORD = 'D',
  XGB_DATA_SIZE_LWORD = 'L'
} xgb_data_size_marking;

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
} xgb_device_type;

typedef struct command_frame {
  uint8_t station_no;  // defined in PLC station number
  uint8_t no_of_blocks;	// no of data blocks to read/send
  xgb_data_size_marking data_size;	// what data size we want to read (amount of bytes we read is no_of_blocks * data_size)
  xgb_device_type device_type;	// data area we want to read
  char *p_device_address;	// device address string - maybe can be done in uint16???
  uint8_t *p_data_buffer;	// adress to data buffer
} command_frame;

__attribute__((packed)) struct respond_ack_frame {
  uint8_t header_ack;
  uint8_t station_no[2];
  uint8_t command;
  uint8_t command_type[2];
  uint8_t no_blocks[2];
  uint8_t no_data[2];
  uint8_t data[245];
  uint8_t tail_eot;
};

__attribute__((packed)) struct respond_nak_frame {
  uint8_t header_nak;
  uint8_t station_no[2];
  uint8_t command;
  uint8_t command_type[2];
  uint8_t no_blocks[2];
  uint8_t error_code[4];
};

__attribute__((packed)) struct request_ind_read_frame {
  uint8_t header_enq;
  uint8_t station_no[2];
  uint8_t command;
  uint8_t command_type[2];
  uint8_t no_blocks[2];
  uint8_t device_lenght[2];
  uint8_t device_name[16];
  uint8_t tail_eot;
};

__attribute__((packed)) struct request_ind_write_frame {
  uint8_t header_enq;
  uint8_t station_no[2];
  uint8_t command;
  uint8_t command_type[2];
  uint8_t no_blocks[2];
  uint8_t device_lenght[2];
  uint8_t device_name[16];
  uint8_t data[229];
  uint8_t tail_eot;
};

__attribute__((packed)) struct request_cont_read_frame {
  uint8_t header_enq;
  uint8_t station_no[2];
  uint8_t command;
  uint8_t command_type[2];
  uint8_t device_lenght[2];
  uint8_t device_name[16];
  uint8_t no_data[2];
  uint8_t tail_eot;
};

static_assert(sizeof(struct respond_ack_frame) <= MAX_FRAME_SIZE,
              "frame struct size exceeded");
static_assert(sizeof(struct respond_nak_frame) <= MAX_FRAME_SIZE,
              "frame struct size exceeded");
static_assert(sizeof(struct transmit_ind_read_frame) <= MAX_FRAME_SIZE,
              "frame struct size exceeded");
static_assert(sizeof(struct transmit_ind_write_frame) <= MAX_FRAME_SIZE,
              "frame struct size exceeded");

// converts marking to amount of bytes
static inline uint8_t data_marking_to_size(xgb_data_size_marking data_size) {
  switch (data_size) {
    case (XGB_DATA_SIZE_BIT): {
      return 1;
    }
    case (XGB_DATA_SIZE_BYTE): {
      return 1;
    }
    case (XGB_DATA_SIZE_WORD): {
      return 2;
    }
    case (XGB_DATA_SIZE_DWORD): {
      return 4;
    }
    case (XGB_DATA_SIZE_LWORD): {
      return 8;
    }
    default: {
      return 0;
    }
  }
}

typedef union frame {
  uint8_t frame_bytes[MAX_FRAME_SIZE];
  struct respond_ack_frame ack_frame;
  struct respond_nak_frame nak_frame;
  struct transmit_ind_read_frame ind_read_frame;
  struct transmit_ind_write_frame ind_write_frame;
  struct request_cont_read_frame cont_read_frame;
} u_frame;

uint8_t xgb_send_individual_read_cmd(const command_frame frame_data);
uint8_t xgb_send_inidividual_write_cmd(const command_frame frame_data);

#endif /* INC_XGB_COMM_H_ */
