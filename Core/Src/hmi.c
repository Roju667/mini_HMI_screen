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
#define HMI_HIGHLIGHT_TILE_COLOR ILI9341_RED

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
volatile hmi_state_t state;
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

// main menu
static void mm_update_tile_cursor_pos(buttons_state_t pending_flag);
static void mm_change_tile_cursor_pos(buttons_state_t pending_flag);
static void mm_check_pending_flags(void);
static void mm_active_screen(void);
static void mm_open_edit_menu(void);

// edit menu
static void em_update_tile_cursor_pos(buttons_state_t pending_flag,
                                      hmi_edit_cursors_t *p_cursors);
static void em_update_switch_cursor_pos(buttons_state_t pending_flag,
                                        hmi_edit_cursors_t *p_cursors);
static void em_change_switch_cursor(buttons_state_t pending_flag,
                                    hmi_edit_cursors_t *p_cursors);
static void em_change_tile_cursor(buttons_state_t pending_flag,
                                  hmi_edit_cursors_t *p_cursors);
static void em_check_pending_flags(hmi_edit_cursors_t *p_cursors);

static void em_active_screen(hmi_edit_cursors_t *p_cursors);

// init
static void init_read_eeprom(void);
static void init_mm_tft(void);

// draw functions
static void draw_cursor(ColorType color);
static void draw_tile(const uint8_t tile_number);
static void draw_main_screen(void);
static void draw_edit_menu(void);
static void draw_wide_tile(const char *text, uint8_t tile_number,
                           bool center_text, ColorType color);
static void draw_std_switch_txt(hmi_edit_cursors_t *p_cursors);

// utility
static uint32_t ut_find_x_to_center_text(const char *text, uint32_t left_border,
                                         uint32_t right_border);
static uint32_t em_get_switch_cursor(hmi_edit_cursors_t *p_cursors);

/*** FUNCTIONS USED OUT OF THIS FILE **/

void hmi_main(void)
{
  state = READ_EEPROM;
  while (1)
    {
      switch (state)
        {
        case (READ_EEPROM):
          {
            init_read_eeprom();
            break;
          }

        case (INIT_TFT):
          {
            init_mm_tft();
            break;
          }

        case (ACTIVE_SCREEN):
          {
            mm_active_screen();
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

static void mm_check_pending_flags(void)
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
          mm_change_tile_cursor_pos(pending_flag);
          break;

        case (ENTER_FLAG):
          mm_open_edit_menu();
          break;
        case (IDLE):
        default:
          break;
        }
    }

  return;
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
          mm_check_pending_flags();
        }
    }
}

// open edit menu to save read/write function to tile
static void mm_open_edit_menu(void)
{

  draw_edit_menu();

  // set cursors
  hmi_edit_cursors_t edit_cursors = {.pos_tile = 1};
  draw_wide_tile(NULL, edit_cursors.pos_tile, false, HMI_HIGHLIGHT_TILE_COLOR);
  em_active_screen(&edit_cursors);
  return;
}

/*** EDIT MENU FUNCTIONS **/

// this function return value of the cursor of current tile
static uint32_t em_get_switch_cursor(hmi_edit_cursors_t *p_cursors)
{
  uint32_t position = 0;
  switch (p_cursors->pos_tile)
    {
    case (1):
      position = p_cursors->pos_fun;
      break;
    case (2):
      position = p_cursors->pos_dev;
      break;
    case (3):
      position = p_cursors->pos_size;
      break;
    }

  return position;
}

// update tile cursor value in structure
static void em_update_tile_cursor_pos(buttons_state_t pending_flag,
                                      hmi_edit_cursors_t *p_cursors)
{

  if (pending_flag == UP_FLAG)
    {
      p_cursors->pos_tile = (p_cursors->pos_tile + 5) % 6;
    }
  else if (pending_flag == DOWN_FLAG)
    {
      p_cursors->pos_tile = (p_cursors->pos_tile + 1) % 6;
    }

  return;
}

// update switch cursor value in structure (switch left/right)
static void em_update_switch_cursor_pos(buttons_state_t pending_flag,
                                        hmi_edit_cursors_t *p_cursors)
{
  if (pending_flag == LEFT_FLAG)
    {
      switch (p_cursors->pos_tile)
        {
        case (1):
          p_cursors->pos_fun = (p_cursors->pos_fun + 1) % 2;
          break;
        case (2):
          p_cursors->pos_dev = (p_cursors->pos_dev + 11) % 12;
          break;
        case (3):
          p_cursors->pos_size = (p_cursors->pos_size + 4) % 5;
          break;
        }
    }
  else if (pending_flag == RIGHT_FLAG)
    {
      switch (p_cursors->pos_tile)
        {
        case (1):
          p_cursors->pos_fun = (p_cursors->pos_fun + 1) % 2;
          break;
        case (2):
          p_cursors->pos_dev = (p_cursors->pos_dev + 1) % 12;
          break;
        case (3):
          p_cursors->pos_size = (p_cursors->pos_size + 1) % 5;
          break;
        }
    }

  return;
}

// change tile cursor position on screen
static void em_change_tile_cursor(buttons_state_t pending_flag,
                                  hmi_edit_cursors_t *p_cursors)
{
  draw_wide_tile(NULL, p_cursors->pos_tile, false, HMI_TILE_COLOR);
  em_update_tile_cursor_pos(pending_flag, p_cursors);
  draw_wide_tile(NULL, p_cursors->pos_tile, false, HMI_HIGHLIGHT_TILE_COLOR);
  buttons_reset_flag(pending_flag);

  return;
}

// change switch cursor position on screen (switch left/right)
static void em_change_switch_cursor(buttons_state_t pending_flag,
                                    hmi_edit_cursors_t *p_cursors)
{
  em_update_switch_cursor_pos(pending_flag, p_cursors);

  if (p_cursors->pos_tile < 4)
    {
      draw_std_switch_txt(p_cursors);
    }

  buttons_reset_flag(pending_flag);

  return;
}

static void em_check_pending_flags(hmi_edit_cursors_t *p_cursors)
{
  buttons_state_t pending_flag = buttons_check_flag();

  if (IDLE != pending_flag)
    {
      switch (pending_flag)
        {
        case (LEFT_FLAG):
        case (RIGHT_FLAG):
          em_change_switch_cursor(pending_flag, p_cursors);
          break;
        case (UP_FLAG):
        case (DOWN_FLAG):
          em_change_tile_cursor(pending_flag, p_cursors);
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

static void init_read_eeprom(void) { state = INIT_TFT; }

// init tft and draw main screen
static void init_mm_tft(void)
{
  ILI9341_Init(&hspi1);
  GFX_SetFont(font_8x5);
  screen.active_button = 0;
  draw_main_screen();
  state = ACTIVE_SCREEN;
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
  sprintf(message, "TILE NUMBER %d", screen.active_button);
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

static void draw_std_switch_txt(hmi_edit_cursors_t *p_cursors)
{
  uint32_t x_pos = ut_find_x_to_center_text(
      std_switch[(p_cursors->pos_tile) - 1][p_cursors->pos_dev].display_text,
      150, 314);
  ;
  uint32_t y_pos =
      ((GAP_Y_BETWEEN_TILES + TITLE_TILE_HEIGHT) * p_cursors->pos_tile) +
      TEXT_Y_OFFSET;

  // select switch cursor depending on tile cursor
  uint32_t switch_cursor = em_get_switch_cursor(p_cursors);
  // clear text
  GFX_DrawFillRectangle(x_pos, y_pos, 100, 8, HMI_EDIT_MENU_COLOR);

  GFX_DrawString(
      x_pos, y_pos,
      std_switch[(p_cursors->pos_tile) - 1][switch_cursor].display_text,
      HMI_TEXT_COLOR);
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
