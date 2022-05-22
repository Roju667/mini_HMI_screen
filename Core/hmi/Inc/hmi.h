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

typedef uint8_t cursor;

typedef enum hmi_change_screen
{
  NO_CHANGE = 0,
  OPEN_EDIT_MENU = 1,
  OPEN_MAIN_MENU = 2,
  SAVE_DATA_TO_TILE = 3
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

typedef enum tile_function
{
  READ = 0,
  WRITE_CONT = 1,
  WRITE_SINGLE = 2
} tile_function_t;

struct frame_data
{
  uint8_t tile_number;
  uint8_t function;
  xgb_device_type_t device_type;
  xgb_data_size_marking_t size_mark;
  char address[6];
};

typedef void (*tile_callback_t)(const struct frame_data *p_data);

typedef struct hmi_tile
{
  bool tile_active;
  char *p_text;
  struct frame_data data;
  tile_callback_t callback;
  int32_t value;
} hmi_tile_t;

enum cursor_tiles
{
  TILE_HEADER = 0,
  TILE_FUNCTION = 1,
  TILE_STD_SWITCH_START = TILE_FUNCTION,
  TILE_LEFT_ALLIGN_START = TILE_FUNCTION,
  TILE_DEVICE = 2,
  TILE_SIZE = 3,
  TILE_STD_SWITCH_END = TILE_SIZE,
  TILE_ADDRESS = 4,
  TILE_LEFT_ALLIGN_END = TILE_ADDRESS,
  TILE_EXIT = 5
};

typedef struct hmi_edit_cursors
{
  enum cursor_tiles vert_tile;
  cursor horiz_fun;
  cursor horiz_dev;
  cursor horiz_size;
  cursor horiz_address;
  cursor vert_address_num;
  cursor horiz_exit;
  char address[6];
  bool is_edit_mode_active;
} hmi_edit_cursors_t;

typedef struct edit_option
{
  char *display_text;
  char frame_letter;
} edit_option_t;

typedef struct hmi_screen
{
  uint8_t active_main_tile;
  hmi_tile_t tiles[10];

} hmi_main_screen_t;

void hmi_main(void);

#endif /* INC_HMI_H_ */
