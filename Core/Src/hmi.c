/*
 * hmi.c
 *
 *  Created on: 15 kwi 2022
 *      Author: ROJEK
 */
#include "hmi.h"

#include "5buttons.h"
#include "GFX_COLOR.h"
#include "ILI9341.h"
#include "fonts.h"
#include "main.h"

#define RETURN_FRAME_TIMEOUT 1000

// title
#define TITLE_TILE_WIDTH 314U
#define TITLE_TILE_HEIGHT 27U
#define OFFSET_Y_TITLE 1U
#define HMI_BUTTON_WIDTH 155U
#define HMI_TILE_HEIGHT 40U

#define OFFSET_X_LEFT_BORDER 3U
#define OFFSET_Y_FIRST_TILE 30U
#define OFFSET_X_SECOND_COLUMN 159U
#define OFFSET_X_CURSOR_POINTER 18U

#define HMI_TILE_COLOR ILI9341_YELLOW
#define HMI_BACKGROUND_COLOR ILI9341_BLACK
#define HMI_TEXT_COLOR ILI9341_YELLOW
#define HMI_CURSOR_COLOR ILI9341_DARKCYAN

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
volatile hmi_state state;
volatile bool frame_returned;
hmi_screen screen;
uint8_t p_data[MAX_FRAME_SIZE];

static void draw_cursor(ColorType color);

// draw functions
// draw main menu at the warm up
static void hmi_draw_main_screen(void) {
  GFX_DrawRectangle(OFFSET_X_LEFT_BORDER, OFFSET_Y_TITLE, TITLE_TILE_WIDTH,
                    TITLE_TILE_HEIGHT, HMI_TILE_COLOR);

  for (uint8_t j = 0; j < 2; j++) {
    for (uint8_t i = 0; i < 5; i++) {
      GFX_DrawRectangle((j * OFFSET_X_SECOND_COLUMN) + OFFSET_X_LEFT_BORDER,
                        (i * 41) + 29, HMI_BUTTON_WIDTH, HMI_TILE_HEIGHT,
                        HMI_TILE_COLOR);
    }
  }

  draw_cursor(ILI9341_DARKCYAN);
  return;
}

// draw new tile with string value
static void hmi_draw_tile(const uint8_t tile_number, const char *text) {
  GFX_DrawFillRectangle(OFFSET_X_LEFT_BORDER, (tile_number * 21) + 30,
                        HMI_BUTTON_WIDTH - 10, 8, HMI_BACKGROUND_COLOR);

  GFX_DrawString(OFFSET_X_LEFT_BORDER, (tile_number * 21) + 30, text,
                 HMI_TEXT_COLOR);

  return;
}

// draw active tile where the cursor is
static void draw_cursor(ColorType color) {
  uint8_t column = screen.active_button / 5;
  uint8_t row = screen.active_button % 5;

  uint32_t x0_pos =
      (column * OFFSET_X_SECOND_COLUMN) + 1 + OFFSET_X_LEFT_BORDER;
  uint32_t y0_pos = (row * 41) + OFFSET_Y_FIRST_TILE + 1;

  uint32_t x1_pos =
      (column * OFFSET_X_SECOND_COLUMN) + 1 + OFFSET_X_LEFT_BORDER;
  uint32_t y1_pos = (row * 41) + OFFSET_Y_FIRST_TILE + HMI_TILE_HEIGHT - 3;

  uint32_t x2_pos = (column * OFFSET_X_SECOND_COLUMN) + 1 +
                    OFFSET_X_LEFT_BORDER + OFFSET_X_CURSOR_POINTER;
  uint32_t y2_pos = (row * 41) + OFFSET_Y_FIRST_TILE - 1 + HMI_TILE_HEIGHT / 2;

  GFX_DrawFillTriangle(x0_pos, y0_pos, x1_pos, y1_pos, x2_pos, y2_pos, color);

  return;
}

static void change_active_tile_number(buttons_state_t pending_flag) {
  if (pending_flag == LEFT_FLAG) {
    screen.active_button = (screen.active_button + 5) % 10;
  } else if (pending_flag == RIGHT_FLAG) {
    screen.active_button = (screen.active_button + 5) % 10;
  } else if (pending_flag == UP_FLAG) {
    screen.active_button =
        (screen.active_button + 4) % 5 + (5 * (screen.active_button / 5));
  } else if (pending_flag == DOWN_FLAG) {
    screen.active_button =
        (screen.active_button + 1) % 5 + (5 * (screen.active_button / 5));
  }

  return;
}

static void change_cursor_position(buttons_state_t pending_flag) {
  // erase active tile
  draw_cursor(HMI_BACKGROUND_COLOR);
  change_active_tile_number(pending_flag);
  // draw new active tile
  draw_cursor(HMI_CURSOR_COLOR);
  buttons_reset_flag(pending_flag);

  return;
}

static void check_pending_flags(void) {
  buttons_state_t pending_flag = buttons_check_flag();

  if (IDLE != pending_flag) {
    switch (pending_flag) {
      case (LEFT_FLAG):
      case (RIGHT_FLAG):
      case (UP_FLAG):
      case (DOWN_FLAG):
        change_cursor_position(pending_flag);
        break;

      case (ENTER_FLAG):
        // enter menu
        break;
      case (IDLE):
      default:
        break;
    }
  }

  return;
}

static void hmi_read_eeprom(void) { state = INIT_TFT; }

// init tft and draw main screen
static void hmi_init_tft(void) {
  ILI9341_Init(&hspi1);
  ILI9341_ClearDisplay(HMI_BACKGROUND_COLOR);
  GFX_SetFont(font_8x5);
  screen.active_button = 0;
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
      check_pending_flags();
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
