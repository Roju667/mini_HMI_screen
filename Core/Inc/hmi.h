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

typedef enum hmi_change_screen
{
  NO_CHANGE = 0,
  OPEN_EDIT_MENU = 1,
  OPEN_MAIN_MENU = 2
} hmi_change_screen_t;

typedef enum hmi_state
{

  READ_EEPROM = 0,
  INIT_TFT = 1,
  INIT_MAIN_MENU = 2,
  MAIN_MENU = 3,
  INIT_EDIT_MENU = 4,
  EDIT_MENU = 5
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

enum edit_tiles
{
  TILE_HEADER = 0,
  TILE_FUNCTION = 1,
  TILE_DEVICE = 2,
  TILE_SIZE = 3,
  TILE_ADDRESS = 4,
  TILE_EXIT = 5
};

typedef struct hmi_edit_cursors
{
  enum edit_tiles pos_tile;
  uint8_t pos_fun;
  uint8_t pos_dev;
  uint8_t pos_size;
  uint8_t pos_address;
  uint8_t pos_address_num;
  uint8_t pos_exit;
  char address[6];
  bool edit_mode;
} hmi_edit_cursors_t;

typedef struct edit_option
{
  char *display_text;
  char frame_letter;
} edit_option_t;

typedef struct hmi_screen
{
  uint8_t active_main_tile;
  hmi_tile_t buttons[10];

} hmi_screen_t;

void hmi_main(void);

#endif /* INC_HMI_H_ */
