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

#define HMI_TITLE_TILE_WIDTH 314U
#define HMI_TITLE_TILE_HEIGHT 27U
#define HMI_BUTTON_WIDTH 155U
#define HMI_TILE_HEIGHT 40U
#define HMI_TILE_COLOR ILI9341_YELLOW

typedef enum hmi_state {

  READ_EEPROM = 0,
  INIT_TFT = 1,
  ACTIVE_SCREEN = 2,
  EDIT_TILE = 3
} hmi_state;

struct read_tile_data {
  uint8_t tile_number;
  xgb_device_type type;
  xgb_data_size_marking size_mark;
  char* address;
};

union tile_data {
  struct read_tile_data read_tile;
};

typedef struct hmi_button {
  bool tile_active;
  char* p_text;
  union tile_data data;
  void (*tile_function)(const union tile_data t_data);
} hmi_tile;

typedef struct hmi_screen {
  uint8_t active_button;
  hmi_tile buttons[10];

} hmi_screen;

void hmi_main(void);

#endif /* INC_HMI_H_ */
