/*
 * hmi.c
 *
 *  Created on: 15 kwi 2022
 *      Author: ROJEK
 */
#include "hmi.h"

#include "GFX_COLOR.h"
#include "ILI9341.h"
#include "fonts.h"
#include "main.h"

#define RETURN_FRAME_TIMEOUT 1000

#define HMI_TITLE_TILE_WIDTH 314U
#define HMI_TITLE_TILE_HEIGHT 27U
#define HMI_BUTTON_WIDTH 155U
#define HMI_TILE_HEIGHT 40U
#define HMI_TILE_COLOR ILI9341_YELLOW
#define HMI_BACKGROUND_COLOR ILI9341_DARKCYAN
#define HMI_TEXT_COLOR ILI9341_YELLOW

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
volatile hmi_state state;
volatile bool frame_returned;
hmi_screen screen;
uint8_t p_data[MAX_FRAME_SIZE];

// draw functions
// draw main menu at the warm up
static void hmi_draw_main_screen(void) {
  GFX_DrawRectangle(3, 1, HMI_TITLE_TILE_WIDTH, HMI_TITLE_TILE_HEIGHT,
                    HMI_TILE_COLOR);

  for (uint8_t j = 0; j < 2; j++) {
    for (uint8_t i = 0; i < 5; i++) {
      GFX_DrawRectangle((j * 159) + 3, (i * 41) + 29, HMI_BUTTON_WIDTH,
                        HMI_TILE_HEIGHT, HMI_TILE_COLOR);
    }
  }
  return;
}

// draw new tile with string value
static void hmi_draw_tile(const uint8_t tile_number, const char *text) {
  GFX_DrawFillRectangle(3, (tile_number * 21) + 30, HMI_BUTTON_WIDTH - 10, 8,
                        HMI_BACKGROUND_COLOR);

  GFX_DrawString(3, (tile_number * 21) + 30, text, HMI_TEXT_COLOR);

  return;
}

// draw active tile where the cursor is
static void hmi_draw_active_tile(uint8_t active_tile_number) {
  GFX_DrawRectangle((j * 159) + 3, (i * 41) + 29, HMI_BUTTON_WIDTH,
                    HMI_TILE_HEIGHT, HMI_TILE_COLOR);
}

static void hmi_read_eeprom(void) { state = INIT_TFT; }

// init tft and draw main screen
static void hmi_init_tft(void) {
  ILI9341_Init(&hspi1);
  ILI9341_ClearDisplay(HMI_BACKGROUND_COLOR);
  GFX_SetFont(font_8x5);
  hmi_draw_main_screen();
  state = ACTIVE_SCREEN;
  return;
}

static void hmi_active_screen(void) {
  while (1) {
    // do every tile callback
    for (uint8_t i = 0; i < 10; i++) {
      if (NULL != screen.buttons[i].callback) {
        screen.buttons[i].callback(&screen.buttons[i].data);
      }

      // check if button was pressed
    }
  }
}

static void hmi_read_tile_function(const union tile_data *p_data) {
  // read on DMA
  frame_returned = false;
  bool timeout_error = false;
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)p_data, MAX_FRAME_SIZE);

  // send read command
  xgb_read_single_device(p_data->read_tile.type, p_data->read_tile.size_mark,
                         p_data->read_tile.address);

  // wait for return frame
  uint32_t current_tick = HAL_GetTick();
  while (false == frame_returned) {
    if (HAL_GetTick() - current_tick > RETURN_FRAME_TIMEOUT) {
      timeout_error = true;
      break;
    }
  }

  if (true == timeout_error) {
    hmi_draw_tile(p_data->read_tile.tile_number, (char *)p_data);
  } else {
    hmi_draw_tile(p_data->read_tile.tile_number, "TIMEOUT");
  }

  return;
}

void hmi_main(void) {
  state = READ_EEPROM;
  while (1) {
    switch (state) {
      case (READ_EEPROM): {
        hmi_read_eeprom();
        break;
      }

      case (INIT_TFT): {
        hmi_init_tft();
        break;
      }

      case (ACTIVE_SCREEN): {
        screen.buttons[0].data.read_tile.address = "0";
        screen.buttons[0].data.read_tile.size_mark = XGB_DATA_SIZE_BYTE;
        screen.buttons[0].data.read_tile.tile_number = 0;
        screen.buttons[0].data.read_tile.type = XGB_DEV_TYPE_P;
        screen.buttons[0].callback = hmi_read_tile_function;
        hmi_active_screen();
        break;
      }

      case (EDIT_TILE): {
        hmi_active_screen();
        break;
      }

      default: {
        // shouldnt happend
      }
    }
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  frame_returned = true;
}
