/*
 * hmi.c
 *
 *  Created on: 15 kwi 2022
 *      Author: ROJEK
 */

#include "main.h"
#include "stdio.h"
#include "string.h"

#include "ILI9341.h"
#include "5buttons.h"

#include "GFX_COLOR.h"
#include "fonts.h"

#include "hmi.h"
#include "hmi_draw.h"

#define SWITCH_SCREEN 1U

#define RETURN_FRAME_TIMEOUT 1000

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
static volatile hmi_state_t hmi_state;
static volatile bool frame_returned;
static hmi_screen_t screen;

// edit menu text

static hmi_edit_cursors_t edit_cursors = {.pos_tile = 1};

// main menu
static void mm_update_tile_cursor_pos(buttons_state_t pending_flag);
static void mm_change_tile_cursor_pos(buttons_state_t pending_flag);
static hmi_change_screen_t mm_check_pending_flags(void);
static void mm_active_screen(void);
static void em_open_edit_menu(void);

// edit menu
static void em_update_vertical_cursor_pos(buttons_state_t pending_flag,
                                          hmi_edit_cursors_t *p_cursors);
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

// utility
static uint32_t ut_find_x_to_center_text(const char *text, uint32_t left_border,
                                         uint32_t right_border);

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
  draw_mm_cursor(HMI_BACKGROUND_COLOR, screen.active_main_tile);
  mm_update_tile_cursor_pos(pending_flag);
  // draw new active tile
  draw_mm_cursor(HMI_CURSOR_COLOR, screen.active_main_tile);
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
  draw_edit_menu(screen.active_main_tile);
  // set cursors
  draw_cursor_initial_values(&edit_cursors);
  draw_wide_tile(NULL, edit_cursors.pos_tile, false, HMI_HIGHLIGHT_TILE_COLOR);
  return;
}

/*** EDIT MENU FUNCTIONS **/

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
  draw_erase_std_switch_txt(p_cursors);
  em_update_horizontal_cursor_pos(pending_flag, p_cursors);
  draw_std_switch_txt(p_cursors,p_cursors->pos_tile);
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
  draw_main_screen(screen.active_main_tile);
  hmi_state = MAIN_MENU;
  return;
}

static void init_edit_menu(void)
{
  em_open_edit_menu();
  hmi_state = EDIT_MENU;
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
      draw_small_tile(p_data->tile_number);
    }
  else
    {
      draw_small_tile(p_data->tile_number);
    }

  return;
}
