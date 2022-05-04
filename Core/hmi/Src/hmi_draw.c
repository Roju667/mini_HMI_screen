/*
 * hmi_draw.c
 *
 *  Created on: 4 maj 2022
 *      Author: ROJEK
 */

#include "ILI9341.h"
#include "hmi.h"
#include "main.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "GFX_COLOR.h"
#include "hmi_draw.h"

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

// Standard switch limits
#define STD_SW_LEFT_LIMIT 150
#define STD_SW_RIGHT_LIMIT 314

static const edit_option_t fun_switch[] = {{"<READ>", 0}, {"<WRITE>", 0}};

static const edit_option_t device_switch[] = {
    {"<P>", XGB_DEV_TYPE_P}, {"<M>", XGB_DEV_TYPE_M}, {"<K>", XGB_DEV_TYPE_K},
    {"<F>", XGB_DEV_TYPE_F}, {"<T>", XGB_DEV_TYPE_T}, {"<C>", XGB_DEV_TYPE_C},
    {"<L>", XGB_DEV_TYPE_L}, {"<N>", XGB_DEV_TYPE_N}, {"<D>", XGB_DEV_TYPE_D},
    {"<U>", XGB_DEV_TYPE_U}, {"<Z>", XGB_DEV_TYPE_Z}, {"<R>", XGB_DEV_TYPE_R}};

static const edit_option_t size_switch[] = {{"<BIT>", XGB_DATA_SIZE_BIT},
                                            {"<BYTE>", XGB_DATA_SIZE_BYTE},
                                            {"<WORD>", XGB_DATA_SIZE_WORD},
                                            {"<DWORD>", XGB_DATA_SIZE_DWORD},
                                            {"<LWORD>", XGB_DATA_SIZE_LWORD}};

// place holder (std switches are in places [1][2][3])
static const edit_option_t null_switch[] = {};

static const edit_option_t *std_switch[] = {null_switch, fun_switch,
                                            device_switch, size_switch};

static uint32_t ut_find_x_to_center_text(const char *text, uint32_t left_border,
                                         uint32_t right_border);

static uint32_t ut_get_switch_cursor(const hmi_edit_cursors_t *p_cursors);

/*** DRAW FUNCTIONS **/

// Draw main menu tile
void draw_small_tile(uint8_t tile_number)
{
  uint8_t column = tile_number / 5;
  uint8_t row = tile_number % 5;

  uint32_t x_pos = (column * OFFSET_X_SECOND_COLUMN) + OFFSET_X_LEFT_BORDER;
  uint32_t y_pos = (row * DISTANCE_Y_BETWEEN_TILES) + OFFSET_Y_FIRST_TILE;

  GFX_DrawRectangle(x_pos, y_pos, TILE_WIDTH, TILE_HEIGHT, HMI_TILE_COLOR);
  return;
}

// Draw wide tile and draw text in it (in center or left-aligned)
void draw_wide_tile(const char *text, uint8_t tile_number, bool center_text,
                    ColorType color)
{
  GFX_DrawRectangle(OFFSET_X_LEFT_BORDER,
                    (GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * tile_number,
                    TITLE_TILE_WIDTH, TITLE_TILE_HEIGHT, color);

  uint32_t x_pos = TEXT_X_OFFSET;
  uint32_t y_pos =
      ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * tile_number) + TEXT_Y_OFFSET;

  if (center_text == true)
    {
      x_pos =
          ut_find_x_to_center_text(text, OFFSET_X_LEFT_BORDER,
                                   (ILI9341_TFTWIDTH - OFFSET_X_LEFT_BORDER));
    }

  if (NULL != text)
    {
      GFX_DrawString(x_pos, y_pos, text, HMI_TEXT_COLOR);
    }

  return;
}

// Draw main menu cursor on active tile
void draw_mm_cursor(ColorType color, uint8_t active_tile)
{
  uint8_t column = active_tile / 5;
  uint8_t row = active_tile % 5;

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

// Draw main menu screen - header tile + 10 read/write tiles
void draw_main_screen(uint8_t active_tile)
{
  ILI9341_ClearDisplay(HMI_BACKGROUND_COLOR);
  draw_wide_tile("XGB PLC COMMUNICATION", 0, true, HMI_TILE_COLOR);
  for (uint8_t i = 0; i < 10; i++)
    {
      draw_small_tile(i);
    }

  draw_mm_cursor(ILI9341_DARKCYAN, active_tile);
  return;
}

// Draw edit menu screen - header tile + 5 config tiles
void draw_edit_menu(uint8_t active_main_tile)
{

  ILI9341_ClearDisplay(HMI_EDIT_MENU_COLOR);

  char message[16] = {0};
  sprintf(message, "TILE NUMBER %d", active_main_tile);

  const char *tile_text[] = {
      message,        "Tile function:",  "Device Type:",
      "Device Size:", "Device Address:", "Confirm - Discard"};

  draw_wide_tile(tile_text[TILE_HEADER], TILE_HEADER, true, HMI_TILE_COLOR);

  for (uint8_t i = TILE_FUNCTION; i < TILE_ADDRESS; i++)
    {
      draw_wide_tile(tile_text[i], i, false, HMI_TILE_COLOR);
    }

  draw_wide_tile(tile_text[TILE_EXIT], TILE_EXIT, true, HMI_TILE_COLOR);

  return;
}

// Draw arrows icon when address edit is selected
void draw_arrows_icon(ColorType color)
{
  uint32_t x_pos =
      ut_find_x_to_center_text("000000", STD_SW_LEFT_LIMIT, STD_SW_RIGHT_LIMIT);

  x_pos = x_pos + (8 * (FONT_WIDTH + FONT_SPACE));

  uint32_t y_pos = ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * TILE_ADDRESS) +
                   TEXT_Y_OFFSET;

  // draw 2 arrows
  GFX_DrawFillTriangle(x_pos, y_pos - 1, x_pos - 4, y_pos + 3, x_pos + 4,
                       y_pos + 3, color);
  GFX_DrawFillTriangle(x_pos, y_pos + 9, x_pos - 4, y_pos + 5, x_pos + 4,
                       y_pos + 5, color);
  return;
}

// Draw digits 0-9 when choosing device address
void draw_address_char(const hmi_edit_cursors_t *p_cursors)
{

  uint32_t x_pos =
      ut_find_x_to_center_text("000000", STD_SW_LEFT_LIMIT, STD_SW_RIGHT_LIMIT);
  ;
  // offset for next letter
  x_pos = x_pos + (p_cursors->pos_address * (FONT_WIDTH + FONT_SPACE));

  uint32_t y_pos = ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * TILE_ADDRESS) +
                   TEXT_Y_OFFSET;

  GFX_DrawFillRectangle(x_pos, y_pos, FONT_WIDTH, FONT_HEIGHT,
                        HMI_EDIT_MENU_COLOR);

  GFX_DrawChar(x_pos, y_pos, p_cursors->pos_address_num + '0', HMI_TEXT_COLOR);

  return;
}

// Draw highlight line below Confirm or Discard option in exit tile
// also used to erase this highlight
void draw_exit_cursor(const hmi_edit_cursors_t *p_cursors, ColorType color)
{

  uint32_t x_pos =
      ut_find_x_to_center_text("Confirm - Discard", OFFSET_X_LEFT_BORDER,
                               (ILI9341_TFTWIDTH - OFFSET_X_LEFT_BORDER));
  ;

  uint32_t x_offset = ((strlen("Confirm - ") * (FONT_WIDTH + FONT_SPACE))) *
                      p_cursors->pos_exit;

  x_pos = x_pos + x_offset;

  uint32_t y_pos = ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * TILE_EXIT) +
                   TEXT_Y_OFFSET + (FONT_HEIGHT + FONT_SPACE);

  uint32_t len = strlen("Confirm") * (FONT_WIDTH + FONT_SPACE);

  GFX_DrawLine(x_pos, y_pos, x_pos + len, y_pos, color);

  return;
}

// Draw highlight line below address chars
// also used to erase this highlight
void draw_address_cursor(const hmi_edit_cursors_t *p_cursors, ColorType color)
{
  uint32_t x_pos =
      ut_find_x_to_center_text("000000", STD_SW_LEFT_LIMIT, STD_SW_RIGHT_LIMIT);
  ;
  // offset for next letter
  x_pos = x_pos + (p_cursors->pos_address * (FONT_WIDTH + FONT_SPACE));

  // position of character
  uint32_t y_pos = ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * TILE_ADDRESS) +
                   TEXT_Y_OFFSET;

  // move it below characater
  y_pos = y_pos + FONT_HEIGHT + 1;

  GFX_DrawLine(x_pos, y_pos, x_pos + FONT_WIDTH, y_pos, color);

  return;
}

// Draw new number when choosing horizontally on header tile
void draw_update_tile_number(char number)
{
  uint32_t x_pos =
      ut_find_x_to_center_text("TILE NUMBER ", OFFSET_X_LEFT_BORDER,
                               (ILI9341_TFTWIDTH - OFFSET_X_LEFT_BORDER));

  x_pos = x_pos + strlen("TILE NUMBER ") * (FONT_WIDTH + FONT_SPACE);

  uint32_t y_pos =
      ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * TILE_HEADER) + TEXT_Y_OFFSET;

  GFX_DrawFillRectangle(x_pos, y_pos, FONT_WIDTH, FONT_HEIGHT,
                        HMI_EDIT_MENU_COLOR);

  GFX_DrawChar(x_pos, y_pos, number, HMI_TEXT_COLOR);

  return;
}

void draw_erase_std_switch_txt(const hmi_edit_cursors_t *p_cursors)
{
  uint32_t std_switch_number = p_cursors->pos_tile;

  // select switch cursor depending on tile cursor
  uint32_t switch_cursor = ut_get_switch_cursor(p_cursors);

  uint32_t lenght_to_erase =
      strlen(std_switch[std_switch_number][switch_cursor].display_text) *
      (FONT_WIDTH + FONT_SPACE);

  uint32_t x_pos = ut_find_x_to_center_text(
      std_switch[std_switch_number][switch_cursor].display_text,
      STD_SW_LEFT_LIMIT, STD_SW_RIGHT_LIMIT);
  ;
  uint32_t y_pos =
      ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * p_cursors->pos_tile) +
      TEXT_Y_OFFSET;

  // clear text
  GFX_DrawFillRectangle(x_pos, y_pos, lenght_to_erase, FONT_HEIGHT,
                        HMI_EDIT_MENU_COLOR);

  return;
}

void draw_std_switch_txt(const hmi_edit_cursors_t *p_cursors,
                         uint8_t switch_number)
{
  // select switch cursor depending on tile cursor
  uint32_t switch_cursor = ut_get_switch_cursor(p_cursors);

  uint32_t x_pos = ut_find_x_to_center_text(
      std_switch[switch_number][switch_cursor].display_text, STD_SW_LEFT_LIMIT,
      STD_SW_RIGHT_LIMIT);
  ;
  uint32_t y_pos = ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * switch_number) +
                   TEXT_Y_OFFSET;

  // select a text from an array of arrays of strings (std_switch)
  GFX_DrawString(x_pos, y_pos,
                 std_switch[switch_number][switch_cursor].display_text,
                 HMI_TEXT_COLOR);

  return;
}

void draw_cursor_initial_values(const hmi_edit_cursors_t *p_cursors)
{
  // standard switches
  for (uint8_t i = TILE_FUNCTION; i < TILE_ADDRESS; i++)
    {
      draw_std_switch_txt(p_cursors, i);
    }

  // address switch
  uint32_t x_pos =
      ut_find_x_to_center_text("000000", STD_SW_LEFT_LIMIT, STD_SW_RIGHT_LIMIT);
  ;
  uint32_t y_pos = ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * TILE_ADDRESS) +
                   TEXT_Y_OFFSET;

  GFX_DrawString(x_pos, y_pos, "000000", HMI_TEXT_COLOR);

  return;
}

/*** UTILITY FUNCTIONS **/

static uint32_t ut_find_x_to_center_text(const char *text, uint32_t left_border,
                                         uint32_t right_border)
{
  uint32_t string_lenght = strlen(text) * (FONT_WIDTH + FONT_SPACE);
  uint32_t tile_width = right_border - left_border;
  uint32_t start_text_pos = ((tile_width - string_lenght) / 2) + left_border;
  return start_text_pos;
}

// this function return value of the cursor of current tile
static uint32_t ut_get_switch_cursor(const hmi_edit_cursors_t *p_cursors)
{
  uint32_t position = 0;
  switch (p_cursors->pos_tile)
    {
    case (TILE_FUNCTION):
      position = p_cursors->pos_fun;
      break;
    case (TILE_DEVICE):
      position = p_cursors->pos_dev;
      break;
    case (TILE_SIZE):
      position = p_cursors->pos_size;
      break;
    case (TILE_HEADER):
    case (TILE_ADDRESS):
    case (TILE_EXIT):
    default:
      break;
    }

  return position;
}
