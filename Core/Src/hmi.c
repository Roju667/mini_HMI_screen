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
#include "stdio.h"
#include "string.h"

#define RETURN_FRAME_TIMEOUT 1000

// Title tile
#define TITLE_TILE_WIDTH 314U
#define TITLE_TILE_HEIGHT 27U

// Gaps
#define GAP_Y_BETWEEN_TILES 1U
#define GAP_X_BETWEEN_COLUMNS 4U
#define OFFSET_X_LEFT_BORDER 3U

// Standard tile
#define TILE_WIDTH 155U
#define TILE_HEIGHT 40U

// Size of line
#define LINE_SIZE 1U

// Distance in Y from the top to the first tile
#define OFFSET_Y_FIRST_TILE                                                    \
  (GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT + GAP_Y_BETWEEN_TILES)

// Distance in X from the left side to start of second column tile
#define OFFSET_X_SECOND_COLUMN (TILE_WIDTH + GAP_X_BETWEEN_COLUMNS)

// Distance in Y between start of one tile to next tile
#define DISTANCE_Y_BETWEEN_TILES (TILE_HEIGHT + GAP_Y_BETWEEN_TILES)

// Top of the pointer
#define OFFSET_X_CURSOR_POINTER 20U

// Font and text dimensions
#define FONT_WIDTH 5U
#define FONT_SPACE 1U
#define FONT_HEIGHT 8U
#define TEXT_X_OFFSET 10U
#define TEXT_Y_OFFSET 10U

// Colors
#define HMI_TILE_COLOR ILI9341_YELLOW
#define HMI_BACKGROUND_COLOR ILI9341_BLACK
#define HMI_TEXT_COLOR ILI9341_YELLOW
#define HMI_CURSOR_COLOR ILI9341_DARKCYAN
#define HMI_EDIT_MENU_COLOR ILI9341_DARKCYAN

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
volatile hmi_state_t state;
volatile bool frame_returned;
hmi_screen_t screen;
uint8_t p_data[MAX_FRAME_SIZE];

// draw functions
static void draw_cursor(ColorType color);
static void draw_tile(const uint8_t tile_number);
static void draw_main_screen(void);
static void draw_edit_menu(void);
static void draw_wide_tile(const char *text, uint8_t tile_number,
                           bool center_text);

// utility
static uint32_t find_x_to_center_text(const char *text, uint32_t left_border,
                                      uint32_t right_border);

// edit menu functions
static void open_edit_menu(void);
static void edit_tile(void);

static void change_active_tile_number(buttons_state_t pending_flag)
{
  if (pending_flag == LEFT_FLAG)
    {
      screen.active_button = (screen.active_button + 5) % 10;
    }
  else if (pending_flag == RIGHT_FLAG)
    {
      screen.active_button = (screen.active_button + 5) % 10;
    }
  else if (pending_flag == UP_FLAG)
    {
      screen.active_button =
          (screen.active_button + 4) % 5 + (5 * (screen.active_button / 5));
    }
  else if (pending_flag == DOWN_FLAG)
    {
      screen.active_button =
          (screen.active_button + 1) % 5 + (5 * (screen.active_button / 5));
    }

  return;
}

static void change_cursor_position_main(buttons_state_t pending_flag)
{
  // erase active tile
  draw_cursor(HMI_BACKGROUND_COLOR);
  change_active_tile_number(pending_flag);
  // draw new active tile
  draw_cursor(HMI_CURSOR_COLOR);
  buttons_reset_flag(pending_flag);

  return;
}

static void check_pending_flags_main(void)
{
  buttons_state_t pending_flag = buttons_check_flag();

  if (IDLE != pending_flag)
    {
      switch (pending_flag)
        {
        case (LEFT_FLAG):
        case (RIGHT_FLAG):
        case (UP_FLAG):
        case (DOWN_FLAG):
          change_cursor_position_main(pending_flag);
          break;

        case (ENTER_FLAG):
          open_edit_menu();
          break;
        case (IDLE):
        default:
          break;
        }
    }

  return;
}

static void change_tile_cursor_edit(buttons_state_t pending_flag,
                                    uint8_t current_pos)
{
}

static void change_switch_cursor_edit(buttons_state_t pending_flag,
                                      hmi_edit_cursors_t *p_cursors)
{
}

static void check_pending_flags_edit(hmi_edit_cursors_t *p_cursors)
{
  buttons_state_t pending_flag = buttons_check_flag();

  if (IDLE != pending_flag)
    {
      switch (pending_flag)
        {
        case (LEFT_FLAG):
        case (RIGHT_FLAG):
          change_switch_cursor_edit(pending_flag, p_cursors);
          break;
        case (UP_FLAG):
        case (DOWN_FLAG):
          change_cursor_position_edit(pending_flag, p_cursors->pos_tile);
          break;

        case (ENTER_FLAG):
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
static void hmi_init_tft(void)
{
  ILI9341_Init(&hspi1);
  GFX_SetFont(font_8x5);
  screen.active_button = 0;
  draw_main_screen();
  state = ACTIVE_SCREEN;
  return;
}

static void hmi_active_screen(void)
{
  while (1)
    {
      // do every tile callback
      for (uint8_t i = 0; i < 10; i++)
        {
          if (NULL != screen.buttons[i].callback)
            {
              screen.buttons[i].callback(&screen.buttons[i].data);
            }

          // check if button was pressed
          check_pending_flags_main();
        }
    }
}

static void hmi_read_tile_function(const struct tile_data *p_data)
{
  // read on DMA
  frame_returned = false;
  bool timeout_error = false;
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)p_data, MAX_FRAME_SIZE);

  // send read command
  xgb_read_single_device(p_data->type, p_data->size_mark, p_data->address);

  // wait for return frame
  uint32_t current_tick = HAL_GetTick();
  while (false == frame_returned)
    {
      if (HAL_GetTick() - current_tick > RETURN_FRAME_TIMEOUT)
        {
          timeout_error = true;
          break;
        }
    }

  if (true == timeout_error)
    {
      draw_tile(p_data->tile_number);
    }
  else
    {
      draw_tile(p_data->tile_number);
    }

  return;
}

void hmi_main(void)
{
  state = READ_EEPROM;
  while (1)
    {
      switch (state)
        {
        case (READ_EEPROM):
          {
            hmi_read_eeprom();
            break;
          }

        case (INIT_TFT):
          {
            hmi_init_tft();
            break;
          }

        case (ACTIVE_SCREEN):
          {
            hmi_active_screen();
            break;
          }

        case (EDIT_TILE):
          {
            hmi_active_screen();
            break;
          }

        default:
          {
            // shouldnt happend
          }
        }
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  frame_returned = true;
}

// draw new tile with string value
static void draw_tile(const uint8_t tile_number)
{
  uint8_t column = tile_number / 5;
  uint8_t row = tile_number % 5;

  uint32_t x_pos = (column * OFFSET_X_SECOND_COLUMN) + OFFSET_X_LEFT_BORDER;
  uint32_t y_pos = (row * DISTANCE_Y_BETWEEN_TILES) + OFFSET_Y_FIRST_TILE;

  GFX_DrawRectangle(x_pos, y_pos, TILE_WIDTH, TILE_HEIGHT, HMI_TILE_COLOR);
  return;
}

// draw active tile where the cursor is
static void draw_cursor(ColorType color)
{
  uint8_t column = screen.active_button / 5;
  uint8_t row = screen.active_button % 5;

  // to draw cursor not on the tile but in the tile i add LINE_SIZE to the
  // offset
  uint32_t x0_pos =
      (column * OFFSET_X_SECOND_COLUMN) + LINE_SIZE + OFFSET_X_LEFT_BORDER;

  uint32_t y0_pos =
      (row * DISTANCE_Y_BETWEEN_TILES) + OFFSET_Y_FIRST_TILE + LINE_SIZE;

  uint32_t x1_pos =
      (column * OFFSET_X_SECOND_COLUMN) + LINE_SIZE + OFFSET_X_LEFT_BORDER;
  uint32_t y1_pos = (row * DISTANCE_Y_BETWEEN_TILES) + OFFSET_Y_FIRST_TILE +
                    TILE_HEIGHT - LINE_SIZE;

  uint32_t x2_pos = (column * OFFSET_X_SECOND_COLUMN) + OFFSET_X_LEFT_BORDER +
                    OFFSET_X_CURSOR_POINTER;

  uint32_t y2_pos = (row * DISTANCE_Y_BETWEEN_TILES) + OFFSET_Y_FIRST_TILE +
                    (TILE_HEIGHT / 2);

  GFX_DrawFillTriangle(x0_pos, y0_pos, x1_pos, y1_pos, x2_pos, y2_pos, color);

  return;
}

static void draw_main_screen(void)
{
  ILI9341_ClearDisplay(HMI_BACKGROUND_COLOR);
  draw_wide_tile("XGB PLC COMMUNICATION", 0, true);
  for (uint8_t i = 0; i < 10; i++)
    {
      draw_tile(i);
    }

  draw_cursor(ILI9341_DARKCYAN);
  return;
}

static uint32_t find_x_to_center_text(const char *text, uint32_t left_border,
                                      uint32_t right_border)
{
  uint32_t string_lenght = strlen(text) * (FONT_WIDTH + FONT_SPACE);
  uint32_t tile_width = right_border - left_border;
  uint32_t start_text_pos = ((tile_width - string_lenght) / 2) + left_border;
  return start_text_pos;
}

// draw edit menu
static void draw_edit_menu(void)
{
  const char *tile_text[] = {"Tile function:", "Device Type:", "Device Size:",
                             "Device Address:", "Confirm - Discard"};

  ILI9341_ClearDisplay(HMI_EDIT_MENU_COLOR);

  char message[32] = {0};
  sprintf(message, "TILE NUMBER %d", screen.active_button);
  draw_wide_tile(message, 0, true);

  for (uint8_t i = 1; i < 5; i++)
    {
      draw_wide_tile(tile_text[i - 1], i, false);
    }
  draw_wide_tile(tile_text[4], 5, true);

  return;
}

static void draw_wide_tile(const char *text, uint8_t tile_number,
                           bool center_text)
{
  GFX_DrawRectangle(OFFSET_X_LEFT_BORDER,
                    (GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * tile_number,
                    TITLE_TILE_WIDTH, TITLE_TILE_HEIGHT, HMI_TILE_COLOR);

  uint32_t x_pos = TEXT_X_OFFSET;

  if (center_text == true)
    {
      x_pos = find_x_to_center_text(text, OFFSET_X_LEFT_BORDER,
                                    (ILI9341_TFTWIDTH - OFFSET_X_LEFT_BORDER));
    }

  GFX_DrawString(x_pos,
                 ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * tile_number) +
                     TEXT_Y_OFFSET,
                 text, HMI_TEXT_COLOR);

  return;
}

// open edit menu to save read/write function to tile
static void open_edit_menu(void)
{
  draw_edit_menu();
  edit_tile();
  return;
}

static void edit_tile(void)
{
  hmi_edit_cursors_t edit_cursors = {.pos_tile = 1};

  const char *fun_switch[] = {"Read", "Write"};
  const char device_switch[] = {XGB_DEV_TYPE_P, XGB_DEV_TYPE_M, XGB_DEV_TYPE_K,
                                XGB_DEV_TYPE_F, XGB_DEV_TYPE_T, XGB_DEV_TYPE_C,
                                XGB_DEV_TYPE_L, XGB_DEV_TYPE_N, XGB_DEV_TYPE_D,
                                XGB_DEV_TYPE_U, XGB_DEV_TYPE_Z, XGB_DEV_TYPE_R};

  const char size_switch[] = {XGB_DATA_SIZE_BIT, XGB_DATA_SIZE_BYTE,
                              XGB_DATA_SIZE_WORD, XGB_DATA_SIZE_DWORD,
                              XGB_DATA_SIZE_LWORD};

  while (1)
    {
    }
}
