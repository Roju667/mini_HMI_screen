/*
 * xgb_comm.h
 *
 *  Created on: Apr 5, 2022
 *      Author: ROJEK
 */

#ifndef INC_XGB_COMM_H_
#define INC_XGB_COMM_H_

#include "stdint.h"

#define MAX_FRAME_SIZE 128

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

/*
 * DATA TYPE
 */
#define XBG_DEVICE_P 'P'
#define XBG_DEVICE_M 'M'
#define XBG_DEVICE_K 'K'

typedef struct send_frame {
  uint8_t station_no;
  uint8_t no_of_blocks;
  uint8_t device_lenght;
  uint8_t p_device_name[16];
  uint8_t number_of_data;
} send_frame;

__attribute__((packed)) struct respond_ack_frame {
  uint8_t header_ack;
  uint8_t station_no;
  uint8_t command;
  uint8_t command_type[2];
  uint8_t no_blocks[2];
  uint8_t no_data[2];
  uint8_t data[64];
};

__attribute__((packed)) struct respond_nak_frame {
  uint8_t header_nak;
  uint8_t station_no;
  uint8_t command;
  uint8_t command_type[2];
  uint8_t no_blocks[2];
  uint8_t error_code[4];
};

__attribute__((packed)) struct transmit_ind_read_frame {
  uint8_t header_enq;
  uint8_t station_no[2];
  uint8_t command;
  uint8_t command_type[2];
  uint8_t no_blocks[2];
  uint8_t device_lenght[2];
  uint8_t device_name[16];
  uint8_t tail_eot;
};

typedef union frame {
  uint8_t frame_bytes[MAX_FRAME_SIZE];
  struct respond_ack_frame ack_frame;
  struct respond_nak_frame nak_frame;
  struct transmit_ind_read_frame ind_read_frame;
} u_frame;

#endif /* INC_XGB_COMM_H_ */
