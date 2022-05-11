/*
 * hmi_draw.c
 *
 *  Created on: 4 maj 2022
 *      Author: ROJEK
 */

#include "hmi.h"
#include "main.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "hmi_draw.h"

#define WIDE_TILE_WIDTH 314U
#define WIDE_TILE_HEIGHT 27U

#define SMALL_TILE_WIDTH 155U
#define SMALL_TILE_HEIGHT 40U

#define GAP_Y_BETWEEN_TILES 1U
#define GAP_X_BETWEEN_COLUMNS 4U
#define OFFSET_X_LEFT_BORDER 3U

#define LINE_SIZE 1U

#define OFFSET_Y_FIRST_TILE                                                    \
  (GAP_Y_BETWEEN_TILES + WIDE_TILE_HEIGHT + GAP_Y_BETWEEN_TILES)

#define OFFSET_X_SECOND_COLUMN (SMALL_TILE_WIDTH + GAP_X_BETWEEN_COLUMNS)

#define DISTANCE_Y_BETWEEN_TILES (SMALL_TILE_HEIGHT + GAP_Y_BETWEEN_TILES)

#define OFFSET_X_CURSOR_POINTER 20U

#define FONT_WIDTH 5U
#define FONT_SPACE 1U
#define FONT_HEIGHT 8U
#define TEXT_X_OFFSET_WIDE_TILE 10U
#define TEXT_Y_OFFSET_WIDE_TILE 10U
#define TEXT_X_OFFSET_SMALL_TILE 18U

#define STD_SW_LEFT_LIMIT 150
#define STD_SW_RIGHT_LIMIT 314

static uint32_t find_x_to_center_text(const char *text, uint32_t left_limit,
                                      uint32_t right_limit);
static uint32_t get_switch_cursor_val(const hmi_edit_cursors_t *p_cursors);
static void draw_initial_address_switch(void);

void draw_small_tile(uint8_t tile_number, const char *text, bool center_text)
{
  uint8_t column = tile_number / 5;
  uint8_t row = tile_number % 5;

  uint32_t x_pos = (column * OFFSET_X_SECOND_COLUMN) + OFFSET_X_LEFT_BORDER;
  uint32_t y_pos = (row * DISTANCE_Y_BETWEEN_TILES) + OFFSET_Y_FIRST_TILE;

  GFX_DrawRectangle(x_pos, y_pos, SMALL_TILE_WIDTH, SMALL_TILE_HEIGHT,
                    HMI_TILE_COLOR);

  draw_small_tile_text(tile_number, text, center_text);
  return;
}

void draw_wide_tile(const char *text, uint8_t tile_number, bool center_text,
                    ColorType color)
{
  GFX_DrawRectangle(OFFSET_X_LEFT_BORDER,
                    (GAP_Y_BETWEEN_TILES + WIDE_TILE_HEIGHT) * tile_number,
                    WIDE_TILE_WIDTH, WIDE_TILE_HEIGHT, color);

  uint32_t x_pos = TEXT_X_OFFSET_WIDE_TILE;
  uint32_t y_pos = ((GAP_Y_BETWEEN_TILES + WIDE_TILE_HEIGHT) * tile_number) +
                   TEXT_Y_OFFSET_WIDE_TILE;

  if (true == center_text)
    {
      x_pos = find_x_to_center_text(text, OFFSET_X_LEFT_BORDER,
                                    (ILI9341_TFTWIDTH - OFFSET_X_LEFT_BORDER));
    }

  if (NULL != text)
    {
      GFX_DrawString(x_pos, y_pos, text, HMI_TEXT_COLOR);
    }

  return;
}

void draw_small_tile_text(uint8_t tile_number, const char *text,
                          bool center_text)
{
  if (NULL == text)
    return;

  uint8_t column = tile_number / 5;
  uint8_t row = tile_number % 5;
  uint32_t left_limit = (column * OFFSET_X_SECOND_COLUMN) +
                        OFFSET_X_LEFT_BORDER + OFFSET_X_CURSOR_POINTER;
  uint32_t right_limit = (column * OFFSET_X_SECOND_COLUMN) +
                         OFFSET_X_LEFT_BORDER + SMALL_TILE_WIDTH;
  uint32_t x_start_draw = left_limit + 1;
  uint32_t y_start_draw = (row * DISTANCE_Y_BETWEEN_TILES) +
                          OFFSET_Y_FIRST_TILE + TEXT_X_OFFSET_SMALL_TILE;
  uint32_t text_width = strlen(text) * (FONT_WIDTH + FONT_SPACE);

  if (true == center_text)
    {
      x_start_draw = find_x_to_center_text(text, left_limit, right_limit);
    }

  GFX_DrawFillRectangle(x_start_draw, y_start_draw, text_width, FONT_HEIGHT,
                        HMI_BACKGROUND_COLOR);

  GFX_DrawString(x_start_draw, y_start_draw, text, HMI_TEXT_COLOR);

  return;
}

void draw_main_menu_cursor(ColorType color, uint8_t active_tile)
{
  uint8_t column = active_tile / 5;
  uint8_t row = active_tile % 5;

  uint32_t x0_pos =
      (column * OFFSET_X_SECOND_COLUMN) + LINE_SIZE + OFFSET_X_LEFT_BORDER;

  uint32_t y0_pos =
      (row * DISTANCE_Y_BETWEEN_TILES) + OFFSET_Y_FIRST_TILE + LINE_SIZE;

  uint32_t x1_pos =
      (column * OFFSET_X_SECOND_COLUMN) + LINE_SIZE + OFFSET_X_LEFT_BORDER;
  uint32_t y1_pos = (row * DISTANCE_Y_BETWEEN_TILES) + OFFSET_Y_FIRST_TILE +
                    SMALL_TILE_HEIGHT - LINE_SIZE;

  uint32_t x2_pos = (column * OFFSET_X_SECOND_COLUMN) + OFFSET_X_LEFT_BORDER +
                    OFFSET_X_CURSOR_POINTER;

  uint32_t y2_pos = (row * DISTANCE_Y_BETWEEN_TILES) + OFFSET_Y_FIRST_TILE +
                    (SMALL_TILE_HEIGHT / 2);

  GFX_DrawFillTriangle(x0_pos, y0_pos, x1_pos, y1_pos, x2_pos, y2_pos, color);

  return;
}

void draw_main_screen(uint8_t active_tile)
{
  ILI9341_ClearDisplay(HMI_BACKGROUND_COLOR);
  draw_wide_tile("XGB PLC COMMUNICATION", 0, true, HMI_TILE_COLOR);
  for (uint8_t i = 0; i < 10; i++)
    {
      draw_small_tile(i, NULL, false);
    }

  draw_main_menu_cursor(HMI_CURSOR_COLOR, active_tile);
  return;
}

void draw_edit_menu(uint8_t active_main_tile)
{

  ILI9341_ClearDisplay(HMI_EDIT_MENU_COLOR);

  char message[16] = {0};
  sprintf(message, "TILE NUMBER %d", active_main_tile);

  const char *tile_text[] = {
      message,        "Tile function:",  "Device Type:",
      "Device Size:", "Device Address:", "Confirm - Discard"};

  draw_wide_tile(tile_text[TILE_HEADER], TILE_HEADER, true, HMI_TILE_COLOR);

  for (uint8_t i = TILE_LEFT_ALLIGN_START; i <= TILE_LEFT_ALLIGN_END; i++)
    {
      draw_wide_tile(tile_text[i], i, false, HMI_TILE_COLOR);
    }

  draw_wide_tile(tile_text[TILE_EXIT], TILE_EXIT, true, HMI_TILE_COLOR);

  return;
}

void draw_arrows_icon(ColorType color)
{
  uint32_t x_icon_pos =
      find_x_to_center_text("000000", STD_SW_LEFT_LIMIT, STD_SW_RIGHT_LIMIT);

  uint32_t selected_tile = (uint32_t)TILE_ADDRESS;

  uint32_t y_icon_pos =
      ((GAP_Y_BETWEEN_TILES + WIDE_TILE_HEIGHT) * selected_tile) +
      TEXT_Y_OFFSET_WIDE_TILE;

  x_icon_pos = x_icon_pos + (8 * (FONT_WIDTH + FONT_SPACE));

  GFX_DrawFillTriangle(x_icon_pos, y_icon_pos - 1, x_icon_pos - 4,
                       y_icon_pos + 3, x_icon_pos + 4, y_icon_pos + 3, color);
  GFX_DrawFillTriangle(x_icon_pos, y_icon_pos + 9, x_icon_pos - 4,
                       y_icon_pos + 5, x_icon_pos + 4, y_icon_pos + 5, color);
  return;
}

void draw_address_char(const hmi_edit_cursors_t *p_cursors)
{

  uint32_t x_char_pos =
      find_x_to_center_text("000000", STD_SW_LEFT_LIMIT, STD_SW_RIGHT_LIMIT);
  ;

  uint32_t selected_char = p_cursors->horiz_address;
  uint32_t selected_tile = (uint32_t)TILE_ADDRESS;
  uint32_t y_pos = ((GAP_Y_BETWEEN_TILES + WIDE_TILE_HEIGHT) * selected_tile) +
                   TEXT_Y_OFFSET_WIDE_TILE;

  char to_draw = p_cursors->vert_address_num + '0';

  x_char_pos = x_char_pos + (selected_char * (FONT_WIDTH + FONT_SPACE));

  GFX_DrawFillRectangle(x_char_pos, y_pos, FONT_WIDTH, FONT_HEIGHT,
                        HMI_EDIT_MENU_COLOR);

  GFX_DrawChar(x_char_pos, y_pos, to_draw, HMI_TEXT_COLOR);

  return;
}

void draw_exit_cursor(const hmi_edit_cursors_t *p_cursors, ColorType color)
{

  uint32_t x_start_line =
      find_x_to_center_text("Confirm - Discard", OFFSET_X_LEFT_BORDER,
                            (ILI9341_TFTWIDTH - OFFSET_X_LEFT_BORDER));
  ;

  uint32_t x_offset = ((strlen("Confirm - ") * (FONT_WIDTH + FONT_SPACE))) *
                      p_cursors->horiz_exit;

  x_start_line = x_start_line + x_offset;

  uint32_t y_start_line =
      ((GAP_Y_BETWEEN_TILES + WIDE_TILE_HEIGHT) * TILE_EXIT) +
      TEXT_Y_OFFSET_WIDE_TILE + (FONT_HEIGHT + FONT_SPACE);

  uint32_t len = strlen("Confirm") * (FONT_WIDTH + FONT_SPACE);

  GFX_DrawLine(x_start_line, y_start_line, x_start_line + len, y_start_line,
               color);

  return;
}

void draw_address_cursor(const hmi_edit_cursors_t *p_cursors, ColorType color)
{
  uint32_t x_start_line =
      find_x_to_center_text("000000", STD_SW_LEFT_LIMIT, STD_SW_RIGHT_LIMIT);
  ;
  uint32_t selected_char = p_cursors->horiz_address;

  x_start_line = x_start_line + (selected_char * (FONT_WIDTH + FONT_SPACE));

  uint32_t y_start_line =
      ((GAP_Y_BETWEEN_TILES + WIDE_TILE_HEIGHT) * TILE_ADDRESS) +
      TEXT_Y_OFFSET_WIDE_TILE;
  y_start_line = y_start_line + FONT_HEIGHT + 1;

  GFX_DrawLine(x_start_line, y_start_line, x_start_line + FONT_WIDTH,
               y_start_line, color);

  return;
}

void draw_update_header_number(char new_number)
{
  uint32_t x_start_draw =
      find_x_to_center_text("TILE NUMBER  ", OFFSET_X_LEFT_BORDER,
                            (ILI9341_TFTWIDTH - OFFSET_X_LEFT_BORDER));

  x_start_draw =
      x_start_draw + strlen("TILE NUMBER ") * (FONT_WIDTH + FONT_SPACE);

  uint32_t y_start_draw =
      ((GAP_Y_BETWEEN_TILES + WIDE_TILE_HEIGHT) * TILE_HEADER) +
      TEXT_Y_OFFSET_WIDE_TILE;

  GFX_DrawFillRectangle(x_start_draw, y_start_draw, FONT_WIDTH, FONT_HEIGHT,
                        HMI_EDIT_MENU_COLOR);

  GFX_DrawChar(x_start_draw, y_start_draw, new_number, HMI_TEXT_COLOR);

  return;
}

void draw_erase_std_switch_text(const hmi_edit_cursors_t *p_cursors,
                                const edit_option_t **p_std_switch)
{
  uint32_t selected_switch = p_cursors->vert_tile;

  uint32_t selected_switch_val = get_switch_cursor_val(p_cursors);

  uint32_t lenght_to_erase =
      strlen(p_std_switch[selected_switch][selected_switch_val].display_text) *
      (FONT_WIDTH + FONT_SPACE);

  uint32_t x_start_erase = find_x_to_center_text(
      p_std_switch[selected_switch][selected_switch_val].display_text,
      STD_SW_LEFT_LIMIT, STD_SW_RIGHT_LIMIT);
  ;
  uint32_t y_start_erase =
      ((GAP_Y_BETWEEN_TILES + WIDE_TILE_HEIGHT) * p_cursors->vert_tile) +
      TEXT_Y_OFFSET_WIDE_TILE;

  GFX_DrawFillRectangle(x_start_erase, y_start_erase, lenght_to_erase,
                        FONT_HEIGHT, HMI_EDIT_MENU_COLOR);

  return;
}

void draw_std_switch_text(const hmi_edit_cursors_t *p_cursors,
                          uint8_t switch_number,
                          const edit_option_t **p_std_switch_array)
{

  uint32_t selected_switch_val = get_switch_cursor_val(p_cursors);

  char *selected_text =
      p_std_switch_array[switch_number][selected_switch_val].display_text;

  uint32_t x_start_text = find_x_to_center_text(
      p_std_switch_array[switch_number][selected_switch_val].display_text,
      STD_SW_LEFT_LIMIT, STD_SW_RIGHT_LIMIT);
  ;
  uint32_t y_start_text =
      ((GAP_Y_BETWEEN_TILES + WIDE_TILE_HEIGHT) * switch_number) +
      TEXT_Y_OFFSET_WIDE_TILE;

  GFX_DrawString(x_start_text, y_start_text, selected_text, HMI_TEXT_COLOR);

  return;
}

void draw_cursors_initial_values(const hmi_edit_cursors_t *p_cursors,
                                 const edit_option_t **p_std_switch)
{
  for (uint8_t i = TILE_STD_SWITCH_START; i <= TILE_STD_SWITCH_END; i++)
    {
      draw_std_switch_text(p_cursors, i, p_std_switch);
    }

  draw_initial_address_switch();

  /* Tile selection cursor */
  draw_wide_tile(NULL, TILE_HEADER, false, HMI_HIGHLIGHT_TILE_COLOR);

  return;
}

static void draw_initial_address_switch(void)
{

  uint32_t tile_position = (uint32_t)TILE_ADDRESS;

  uint32_t x_start_text =
      find_x_to_center_text("000000", STD_SW_LEFT_LIMIT, STD_SW_RIGHT_LIMIT);
  ;
  uint32_t y_start_text =
      ((GAP_Y_BETWEEN_TILES + WIDE_TILE_HEIGHT) * tile_position) +
      TEXT_Y_OFFSET_WIDE_TILE;

  GFX_DrawString(x_start_text, y_start_text, "000000", HMI_TEXT_COLOR);

  return;
}

static uint32_t find_x_to_center_text(const char *text, uint32_t left_limit,
                                      uint32_t right_limit)
{
  uint32_t string_lenght = strlen(text) * (FONT_WIDTH + FONT_SPACE);
  uint32_t tile_width = right_limit - left_limit;
  uint32_t start_text_pos = ((tile_width - string_lenght) / 2) + left_limit;
  return start_text_pos;
}

static uint32_t get_switch_cursor_val(const hmi_edit_cursors_t *p_cursors)
{
  uint32_t position = 0;
  switch (p_cursors->vert_tile)
    {
    case (TILE_FUNCTION):
      position = p_cursors->horiz_fun;
      break;
    case (TILE_DEVICE):
      position = p_cursors->horiz_dev;
      break;
    case (TILE_SIZE):
      position = p_cursors->horiz_size;
      break;
    case (TILE_HEADER):
      /* FALLTHORUGH */
    case (TILE_ADDRESS):
      /* FALLTHORUGH */
    case (TILE_EXIT):
      /* FALLTHORUGH */
    default:
      break;
    }

  return position;
}
