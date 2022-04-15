/*
 * hmi.c
 *
 *  Created on: 15 kwi 2022
 *      Author: ROJEK
 */


#include "main.h"
#include "GFX_COLOR.h"
#include "ILI9341.h"
#include "fonts.h"
#include "hmi.h"

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
volatile hmi_state state;
volatile uint8_t frame_returned;
hmi_screen screen;
uint8_t p_data[MAX_FRAME_SIZE];

static void hmi_draw_main_screen(void) {
  GFX_DrawRectangle(3, 1, HMI_TITLE_TILE_WIDTH, HMI_TITLE_TILE_HEIGHT,
                    HMI_TILE_COLOR);

  for (uint8_t j = 0; j < 2; j++) {
    for (uint8_t i = 0; i < 5; i++) {
      GFX_DrawRectangle((j * 159) + 3, (i * 41) + 29, HMI_BUTTON_WIDTH,
                        HMI_TILE_HEIGHT, HMI_TILE_COLOR);
    }
  }

  GFX_SetFont(font_8x5);
  state = ACTIVE_SCREEN;
  return;
}

static void hmi_read_eeprom(void) { state = INIT_TFT; }

static void hmi_init_tft(void) {
  ILI9341_Init(&hspi1);
  ILI9341_ClearDisplay(ILI9341_DARKCYAN);
  hmi_draw_main_screen();
}

static void hmi_active_screen(void) {
  while (1) {
    for (uint8_t i = 0; i < 10; i++) {
      if (screen.buttons[i].tile_function != NULL) {
        screen.buttons[i].tile_function(screen.buttons[i].data);
      }
    }
  }
}

void hmi_read_tile_function(const union tile_data data) {
  frame_returned = 0;
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, p_data, MAX_FRAME_SIZE);

  xgb_read_single_device(data.read_tile.type, data.read_tile.size_mark,
                         data.read_tile.address);

  // wait
  while (!frame_returned) {
  }

  GFX_DrawFillRectangle(3, data.read_tile.tile_number * 21 + 30, HMI_BUTTON_WIDTH - 10, 8, ILI9341_DARKCYAN);

  GFX_DrawString(3, data.read_tile.tile_number * 21 + 30, (char *)p_data,
                 ILI9341_YELLOW);
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
        screen.buttons[0].tile_function = hmi_read_tile_function;
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
  frame_returned = 1;
}
