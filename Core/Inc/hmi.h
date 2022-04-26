/*
 * hmi.h
 *
 *  Created on: 15 kwi 2022
 *      Author: ROJEK
 */

#ifndef INC_HMI_H_
#define INC_HMI_H_

#include "stdbool.h"
#include "xgb_comm.h"

typedef enum hmi_state
{

  READ_EEPROM = 0,
  INIT_TFT = 1,
  ACTIVE_SCREEN = 2,
  EDIT_TILE = 3
} hmi_state_t;

struct tile_data
{
  uint8_t tile_number;
  uint8_t function;
  xgb_device_type type;
  xgb_data_size_marking size_mark;
  char *address;
};

typedef void (*tile_callback_t)(const struct tile_data *p_data);

typedef struct hmi_tile
{
  bool tile_active;
  char *p_text;
  struct tile_data data;
  tile_callback_t callback;
} hmi_tile_t;

typedef struct hmi_edit_cursors
{
  uint8_t pos_tile;
  uint8_t pos_fun;
  uint8_t pos_dev;
  uint8_t pos_size;
  uint8_t pos_address;
  uint8_t pos_address_num;
} hmi_edit_cursors_t;

typedef struct edit_option
{
  char *display_text;
  char frame_letter;
} edit_option_t;

typedef struct hmi_screen
{
  uint8_t active_button;
  hmi_tile_t buttons[10];

} hmi_screen_t;

void hmi_main(void);

#endif /* INC_HMI_H_ */
