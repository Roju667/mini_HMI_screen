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

#define SWITCH_SCREEN 1U

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
#define HMI_HIGHLIGHT_TILE_COLOR ILI9341_RED

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
volatile hmi_state_t hmi_state;
volatile bool frame_returned;
hmi_screen_t screen;
uint8_t p_data[MAX_FRAME_SIZE];

// edit menu text
const edit_option_t fun_switch[] = {{"<READ>", 0}, {"<WRITE>", 0}};

const edit_option_t device_switch[] = {
    {"<P>", XGB_DEV_TYPE_P}, {"<M>", XGB_DEV_TYPE_M}, {"<K>", XGB_DEV_TYPE_K},
    {"<F>", XGB_DEV_TYPE_F}, {"<T>", XGB_DEV_TYPE_T}, {"<C>", XGB_DEV_TYPE_C},
    {"<L>", XGB_DEV_TYPE_L}, {"<N>", XGB_DEV_TYPE_N}, {"<D>", XGB_DEV_TYPE_D},
    {"<U>", XGB_DEV_TYPE_U}, {"<Z>", XGB_DEV_TYPE_Z}, {"<R>", XGB_DEV_TYPE_R}};

const edit_option_t size_switch[] = {{"<BIT>", XGB_DATA_SIZE_BIT},
                                     {"<BYTE>", XGB_DATA_SIZE_BYTE},
                                     {"<WORD>", XGB_DATA_SIZE_WORD},
                                     {"<DWORD>", XGB_DATA_SIZE_DWORD},
                                     {"<LWORD>", XGB_DATA_SIZE_LWORD}};

const edit_option_t *std_switch[] = {fun_switch, device_switch, size_switch};

hmi_edit_cursors_t edit_cursors = {.pos_tile = 1};

// main menu
static void mm_update_tile_cursor_pos(buttons_state_t pending_flag);
static void mm_change_tile_cursor_pos(buttons_state_t pending_flag);
static hmi_change_screen_t mm_check_pending_flags(void);
static void mm_active_screen(void);
static void em_open_edit_menu(void);

// edit menu
static void em_update_vertical_cursor_pos(buttons_state_t pending_flag,
                                          hmi_edit_cursors_t *p_cursors);
static void em_show_cursor_initial_value(hmi_edit_cursors_t *p_cursors);
static void em_update_horizontal_cursor_pos(buttons_state_t pending_flag,
                                            hmi_edit_cursors_t *p_cursors);
static void em_change_horizontal_cursor(buttons_state_t pending_flag,
                                        hmi_edit_cursors_t *p_cursors);
static void em_change_vertical_cursor(buttons_state_t pending_flag,
                                      hmi_edit_cursors_t *p_cursors);
static void em_change_vertical_tile(buttons_state_t pending_flag,
                                    hmi_edit_cursors_t *p_cursors);
static void em_change_vertical_address(buttons_state_t pending_flag,
                                       hmi_edit_cursors_t *p_cursors);
static void em_enter_action(buttons_state_t pending_flag,
                            hmi_edit_cursors_t *p_cursors);

static void em_update_std_switch(buttons_state_t pending_flag,
                                 hmi_edit_cursors_t *p_cursors);
static void em_update_header(buttons_state_t pending_flag,
                             hmi_edit_cursors_t *p_cursors);
static void em_update_address_switch(buttons_state_t pending_flag,
                                     hmi_edit_cursors_t *p_cursors);
static void em_update_exit_switch(buttons_state_t pending_flag,
                                  hmi_edit_cursors_t *p_cursors);

static uint32_t em_check_pending_flags(hmi_edit_cursors_t *p_cursors);

static void em_active_screen(hmi_edit_cursors_t *p_cursors);

// init
static void init_read_eeprom(void);
static void init_tft(void);
static void init_main_menu(void);
static void init_edit_menu(void);

// draw functions
static void draw_cursor(ColorType color);
static void draw_tile(const uint8_t tile_number);
static void draw_main_screen(void);
static void draw_edit_menu(void);
static void draw_wide_tile(const char *text, uint8_t tile_number,
                           bool center_text, ColorType color);
static void draw_std_switch_txt(hmi_edit_cursors_t *p_cursors,
                                uint8_t switch_number);
static void erase_std_switch_txt(hmi_edit_cursors_t *p_cursors);
static void draw_address_cursor(hmi_edit_cursors_t *p_cursors, ColorType color);
static void draw_arrows_icon(ColorType color);
static void draw_address_char(hmi_edit_cursors_t *p_cursors);
static void draw_exit_cursor(hmi_edit_cursors_t *p_cursors, ColorType color);
static void draw_update_tile_number(char number);

// utility
static uint32_t ut_find_x_to_center_text(const char *text, uint32_t left_border,
                                         uint32_t right_border);
static uint32_t em_get_switch_cursor(hmi_edit_cursors_t *p_cursors);

/*** FUNCTIONS USED OUT OF THIS FILE **/

void hmi_main(void)
{
  hmi_state = READ_EEPROM;
  while (1)
    {
      switch (hmi_state)
        {
        case (READ_EEPROM):
          {
            init_read_eeprom();
            break;
          }

        case (INIT_TFT):
          {
            init_tft();
            break;
          }

        case (INIT_MAIN_MENU):
          {
            init_main_menu();
            break;
          }

        case (MAIN_MENU):
          {
            mm_active_screen();
            break;
          }

        case (INIT_EDIT_MENU):
          {
            init_edit_menu();
            break;
          }

        case (EDIT_MENU):
          {
            em_active_screen(&edit_cursors);
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

/*** MAIN MENU FUNCTIONS **/

// update the cursor pos number in structure
static void mm_update_tile_cursor_pos(buttons_state_t pending_flag)
{
  if (pending_flag == LEFT_FLAG)
    {
      screen.active_main_tile = (screen.active_main_tile + 5) % 10;
    }
  else if (pending_flag == RIGHT_FLAG)
    {
      screen.active_main_tile = (screen.active_main_tile + 5) % 10;
    }
  else if (pending_flag == UP_FLAG)
    {
      screen.active_main_tile = (screen.active_main_tile + 4) % 5 +
                                (5 * (screen.active_main_tile / 5));
    }
  else if (pending_flag == DOWN_FLAG)
    {
      screen.active_main_tile = (screen.active_main_tile + 1) % 5 +
                                (5 * (screen.active_main_tile / 5));
    }

  return;
}

// change cursor position on screen
static void mm_change_tile_cursor_pos(buttons_state_t pending_flag)
{
  // erase active tile
  draw_cursor(HMI_BACKGROUND_COLOR);
  mm_update_tile_cursor_pos(pending_flag);
  // draw new active tile
  draw_cursor(HMI_CURSOR_COLOR);
  buttons_reset_flag(pending_flag);

  return;
}

static hmi_change_screen_t mm_check_pending_flags(void)
{
  buttons_state_t pending_flag = buttons_check_flag();

  hmi_change_screen_t change_screen = NO_CHANGE;

  if (IDLE != pending_flag)
    {
      switch (pending_flag)
        {
        case (LEFT_FLAG):
        case (RIGHT_FLAG):
        case (UP_FLAG):
        case (DOWN_FLAG):
          mm_change_tile_cursor_pos(pending_flag);
          break;

        case (ENTER_FLAG):
          change_screen = OPEN_EDIT_MENU;
          break;
        case (IDLE):
        default:
          break;
        }
    }

  return change_screen;
}

static void mm_active_screen(void)
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
          if (OPEN_EDIT_MENU == mm_check_pending_flags())
            {
              hmi_state = INIT_EDIT_MENU;
              return;
            }
        }
    }
}

// open edit menu to save read/write function to tile
static void em_open_edit_menu(void)
{
  draw_edit_menu();
  // set cursors
  em_show_cursor_initial_value(&edit_cursors);
  draw_wide_tile(NULL, edit_cursors.pos_tile, false, HMI_HIGHLIGHT_TILE_COLOR);
  return;
}

/*** EDIT MENU FUNCTIONS **/

// this function return value of the cursor of current tile
static uint32_t em_get_switch_cursor(hmi_edit_cursors_t *p_cursors)
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

static void em_show_cursor_initial_value(hmi_edit_cursors_t *p_cursors)
{
  // standard switches
  for (uint8_t i = 0; i < 3; i++)
    {
      draw_std_switch_txt(p_cursors, i);
    }

  // address switch
  uint32_t x_pos = ut_find_x_to_center_text("000000", 150, 314);
  ;
  uint32_t y_pos =
      ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * 4) + TEXT_Y_OFFSET;

  GFX_DrawString(x_pos, y_pos, "000000", HMI_TEXT_COLOR);

  return;
}

// update tile cursor value in structure
static void em_update_vertical_cursor_pos(buttons_state_t pending_flag,
                                          hmi_edit_cursors_t *p_cursors)
{

  if (pending_flag == UP_FLAG)
    {
      // check if we are changing tile or letter in the address
      if (p_cursors->pos_tile == TILE_ADDRESS && p_cursors->edit_mode == true)
        {
          p_cursors->pos_address_num = (p_cursors->pos_address_num + 9) % 10;
        }
      else
        {
          p_cursors->pos_tile = (p_cursors->pos_tile + 5) % 6;
        }
    }
  else if (pending_flag == DOWN_FLAG)
    {
      if (p_cursors->pos_tile == TILE_ADDRESS && p_cursors->edit_mode == true)
        {
          p_cursors->pos_address_num = (p_cursors->pos_address_num + 1) % 10;
        }
      else
        {
          p_cursors->pos_tile = (p_cursors->pos_tile + 1) % 6;
        }
    }

  return;
}

// update switch cursor value in structure (switch left/right)
static void em_update_horizontal_cursor_pos(buttons_state_t pending_flag,
                                            hmi_edit_cursors_t *p_cursors)
{
  if (pending_flag == LEFT_FLAG)
    {
      switch (p_cursors->pos_tile)
        {
        case (TILE_FUNCTION):
          p_cursors->pos_fun = (p_cursors->pos_fun + 1) % 2;
          break;
        case (TILE_DEVICE):
          p_cursors->pos_dev = (p_cursors->pos_dev + 11) % 12;
          break;
        case (TILE_SIZE):
          p_cursors->pos_size = (p_cursors->pos_size + 4) % 5;
          break;
        case (TILE_ADDRESS):
          p_cursors->pos_address = (p_cursors->pos_address + 5) % 6;
          break;
        case (TILE_EXIT):
          p_cursors->pos_exit = (p_cursors->pos_exit + 1) % 2;
          break;
        case (TILE_HEADER):

        default:
          break;
        }
    }
  else if (pending_flag == RIGHT_FLAG)
    {
      switch (p_cursors->pos_tile)
        {
        case (TILE_FUNCTION):
          p_cursors->pos_fun = (p_cursors->pos_fun + 1) % 2;
          break;
        case (TILE_DEVICE):
          p_cursors->pos_dev = (p_cursors->pos_dev + 1) % 12;
          break;
        case (TILE_SIZE):
          p_cursors->pos_size = (p_cursors->pos_size + 1) % 5;
          break;
        case (TILE_ADDRESS):
          p_cursors->pos_address = (p_cursors->pos_address + 1) % 6;
          break;
        case (TILE_EXIT):
          p_cursors->pos_exit = (p_cursors->pos_exit + 1) % 2;
          break;
        case (TILE_HEADER):
        default:
          break;
        }
    }

  return;
}

static void em_change_vertical_tile(buttons_state_t pending_flag,
                                    hmi_edit_cursors_t *p_cursors)
{
  draw_wide_tile(NULL, p_cursors->pos_tile, false, HMI_TILE_COLOR);
  em_update_vertical_cursor_pos(pending_flag, p_cursors);
  draw_wide_tile(NULL, p_cursors->pos_tile, false, HMI_HIGHLIGHT_TILE_COLOR);

  if (p_cursors->pos_tile == TILE_ADDRESS)
    {
      draw_address_cursor(p_cursors, HMI_TEXT_COLOR);
    }
  else
    {
      draw_address_cursor(p_cursors, HMI_EDIT_MENU_COLOR);
    }

  if (p_cursors->pos_tile == TILE_EXIT)
    {
      draw_exit_cursor(p_cursors, HMI_TEXT_COLOR);
    }
  else
    {
      draw_exit_cursor(p_cursors, HMI_EDIT_MENU_COLOR);
    }

  return;
}

static void em_change_vertical_address(buttons_state_t pending_flag,
                                       hmi_edit_cursors_t *p_cursors)
{
  em_update_vertical_cursor_pos(pending_flag, p_cursors);
  draw_address_char(p_cursors);
  p_cursors->address[p_cursors->pos_address] = (char)p_cursors->pos_address_num;

  return;
}

// change tile cursor position on screen
static void em_change_vertical_cursor(buttons_state_t pending_flag,
                                      hmi_edit_cursors_t *p_cursors)
{

  // edit address
  if (p_cursors->pos_tile == TILE_ADDRESS && p_cursors->edit_mode == true)
    {
      em_change_vertical_address(pending_flag, p_cursors);
    }
  else
    {
      em_change_vertical_tile(pending_flag, p_cursors);
    }

  buttons_reset_flag(pending_flag);

  return;
}

// change switch cursor position on screen (switch left/right)
static void em_change_horizontal_cursor(buttons_state_t pending_flag,
                                        hmi_edit_cursors_t *p_cursors)
{
  switch (p_cursors->pos_tile)
    {
    case (TILE_HEADER):
      em_update_header(pending_flag, p_cursors);
      break;

    case (TILE_DEVICE):
    case (TILE_FUNCTION):
    case (TILE_SIZE):
      em_update_std_switch(pending_flag, p_cursors);
      break;

    case (TILE_ADDRESS):
      em_update_address_switch(pending_flag, p_cursors);
      break;

    case (TILE_EXIT):
      em_update_exit_switch(pending_flag, p_cursors);
      break;
    }

  buttons_reset_flag(pending_flag);

  return;
}

static void em_update_std_switch(buttons_state_t pending_flag,
                                 hmi_edit_cursors_t *p_cursors)
{
  erase_std_switch_txt(p_cursors);
  em_update_horizontal_cursor_pos(pending_flag, p_cursors);
  draw_std_switch_txt(p_cursors, (p_cursors->pos_tile - 1));
  return;
}

static void em_update_header(buttons_state_t pending_flag,
                             hmi_edit_cursors_t *p_cursors)
{
  if (RIGHT_FLAG == pending_flag)
    {
      screen.active_main_tile = (screen.active_main_tile + 1) % 10;
    }
  else if (LEFT_FLAG == pending_flag)
    {
      screen.active_main_tile = (screen.active_main_tile + 9) % 10;
    }

  draw_update_tile_number(screen.active_main_tile + '0');

  return;
}

static void em_update_address_switch(buttons_state_t pending_flag,
                                     hmi_edit_cursors_t *p_cursors)
{
  draw_address_cursor(p_cursors, HMI_EDIT_MENU_COLOR);
  em_update_horizontal_cursor_pos(pending_flag, p_cursors);

  // load stored value into cursor
  p_cursors->pos_address_num =
      (uint8_t)p_cursors->address[p_cursors->pos_address];
  if (p_cursors->edit_mode == true)
    {
      draw_address_cursor(p_cursors, HMI_HIGHLIGHT_TILE_COLOR);
    }
  else
    {
      draw_address_cursor(p_cursors, HMI_TEXT_COLOR);
    }
  return;
}

static void em_update_exit_switch(buttons_state_t pending_flag,
                                  hmi_edit_cursors_t *p_cursors)
{
  draw_exit_cursor(p_cursors, HMI_EDIT_MENU_COLOR);
  em_update_horizontal_cursor_pos(pending_flag, p_cursors);
  draw_exit_cursor(p_cursors, HMI_TEXT_COLOR);
  return;
}

static void em_enter_action(buttons_state_t pending_flag,
                            hmi_edit_cursors_t *p_cursors)
{

  if (p_cursors->pos_tile == TILE_ADDRESS)
    {
      p_cursors->edit_mode = !(p_cursors->edit_mode);

      // show or hide arrows
      if (p_cursors->edit_mode == true)
        {
          draw_arrows_icon(HMI_TEXT_COLOR);
          draw_address_cursor(p_cursors, HMI_HIGHLIGHT_TILE_COLOR);
        }
      else
        {
          draw_arrows_icon(HMI_EDIT_MENU_COLOR);
          draw_address_cursor(p_cursors, HMI_TEXT_COLOR);
        }
    }

  buttons_reset_flag(pending_flag);
}

static uint32_t em_check_pending_flags(hmi_edit_cursors_t *p_cursors)
{
  buttons_state_t pending_flag = buttons_check_flag();

  if (IDLE != pending_flag)
    {
      switch (pending_flag)
        {
        case (LEFT_FLAG):
        case (RIGHT_FLAG):
          em_change_horizontal_cursor(pending_flag, p_cursors);
          break;
        case (UP_FLAG):
        case (DOWN_FLAG):
          em_change_vertical_cursor(pending_flag, p_cursors);
          break;

        case (ENTER_FLAG):
          em_enter_action(pending_flag, p_cursors);
          break;
        case (IDLE):
        default:
          break;
        }
    }

  return;
}

static void em_active_screen(hmi_edit_cursors_t *p_cursors)
{

  // do not need second struct part, but to keep switch options all the same
  // i use this struct

  while (1)
    {
      em_check_pending_flags(p_cursors);
    }
}

/*** INIT FUNCTIONS **/

static void init_read_eeprom(void) { hmi_state = INIT_TFT; }

// init tft and draw main screen
static void init_tft(void)
{
  ILI9341_Init(&hspi1);
  GFX_SetFont(font_8x5);
  hmi_state = INIT_MAIN_MENU;

  return;
}

static void init_main_menu(void)
{
  screen.active_main_tile = 0;
  draw_main_screen();
  hmi_state = MAIN_MENU;
  return;
}

static void init_edit_menu(void)
{
  em_open_edit_menu();
  hmi_state = EDIT_MENU;
  return;
}

/*** DRAW FUNCTIONS **/

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
  uint8_t column = screen.active_main_tile / 5;
  uint8_t row = screen.active_main_tile % 5;

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
  draw_wide_tile("XGB PLC COMMUNICATION", 0, true, HMI_TILE_COLOR);
  for (uint8_t i = 0; i < 10; i++)
    {
      draw_tile(i);
    }

  draw_cursor(ILI9341_DARKCYAN);
  return;
}

// draw edit menu
static void draw_edit_menu(void)
{
  const char *tile_text[] = {"Tile function:", "Device Type:", "Device Size:",
                             "Device Address:", "Confirm - Discard"};

  ILI9341_ClearDisplay(HMI_EDIT_MENU_COLOR);

  char message[32] = {0};
  sprintf(message, "TILE NUMBER %d", screen.active_main_tile);
  draw_wide_tile(message, 0, true, HMI_TILE_COLOR);

  for (uint8_t i = 1; i < 5; i++)
    {
      draw_wide_tile(tile_text[i - 1], i, false, HMI_TILE_COLOR);
    }
  draw_wide_tile(tile_text[4], 5, true, HMI_TILE_COLOR);

  return;
}

static void draw_wide_tile(const char *text, uint8_t tile_number,
                           bool center_text, ColorType color)
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

static void erase_std_switch_txt(hmi_edit_cursors_t *p_cursors)
{
  // tile 1 is switch 0 etc.
  uint32_t std_switch_number = (p_cursors->pos_tile) - 1;

  // select switch cursor depending on tile cursor
  uint32_t switch_cursor = em_get_switch_cursor(p_cursors);

  uint32_t lenght_to_erase =
      strlen(std_switch[std_switch_number][switch_cursor].display_text) *
      (FONT_WIDTH + FONT_SPACE);

  uint32_t x_pos = ut_find_x_to_center_text(
      std_switch[std_switch_number][switch_cursor].display_text, 150, 314);
  ;
  uint32_t y_pos =
      ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * p_cursors->pos_tile) +
      TEXT_Y_OFFSET;

  // clear text
  GFX_DrawFillRectangle(x_pos, y_pos, lenght_to_erase, FONT_HEIGHT,
                        HMI_EDIT_MENU_COLOR);

  return;
}

static void draw_std_switch_txt(hmi_edit_cursors_t *p_cursors,
                                uint8_t switch_number)
{
  // select switch cursor depending on tile cursor
  uint32_t switch_cursor = em_get_switch_cursor(p_cursors);

  uint32_t x_pos = ut_find_x_to_center_text(
      std_switch[switch_number][switch_cursor].display_text, 150, 314);
  ;
  uint32_t y_pos =
      ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * (switch_number + 1)) +
      TEXT_Y_OFFSET;

  GFX_DrawString(x_pos, y_pos,
                 std_switch[switch_number][switch_cursor].display_text,
                 HMI_TEXT_COLOR);

  return;
}

static void draw_arrows_icon(ColorType color)
{
  uint32_t x_pos = ut_find_x_to_center_text("000000", 150, 314);

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

static void draw_address_cursor(hmi_edit_cursors_t *p_cursors, ColorType color)
{
  uint32_t x_pos = ut_find_x_to_center_text("000000", 150, 314);
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

static void draw_address_char(hmi_edit_cursors_t *p_cursors)
{

  uint32_t x_pos = ut_find_x_to_center_text("000000", 150, 314);
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

static void draw_exit_cursor(hmi_edit_cursors_t *p_cursors, ColorType color)
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

static void draw_update_tile_number(char number)
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

/*** UTILITY FUNCTIONS **/

static uint32_t ut_find_x_to_center_text(const char *text, uint32_t left_border,
                                         uint32_t right_border)
{
  uint32_t string_lenght = strlen(text) * (FONT_WIDTH + FONT_SPACE);
  uint32_t tile_width = right_border - left_border;
  uint32_t start_text_pos = ((tile_width - string_lenght) / 2) + left_border;
  return start_text_pos;
}
