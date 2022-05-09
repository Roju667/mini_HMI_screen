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
#include "hmi_edit_menu.h"

#define SWITCH_SCREEN 1U

#define RETURN_FRAME_TIMEOUT 1000

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
static volatile hmi_state_t hmi_state;
static volatile bool frame_returned;
static hmi_main_screen_t main_screen_data;

// main menu
static void mm_update_tile_cursor_pos(buttons_state_t pending_flag);
static void mm_change_tile_cursor_pos(buttons_state_t pending_flag);
static hmi_change_screen_t mm_check_pending_flags(void);

// main functions
static void init_read_eeprom(void);
static void init_tft(void);
static void init_main_menu(void);
static void main_menu_active(void);
static void init_edit_menu(void);
static void edit_menu_active(void);

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
            main_menu_active();
            break;
          }

        case (INIT_EDIT_MENU):
          {
            init_edit_menu();
            break;
          }

        case (EDIT_MENU):
          {
            edit_menu_active();
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
      main_screen_data.active_main_tile =
          (main_screen_data.active_main_tile + 5) % 10;
    }
  else if (pending_flag == RIGHT_FLAG)
    {
      main_screen_data.active_main_tile =
          (main_screen_data.active_main_tile + 5) % 10;
    }
  else if (pending_flag == UP_FLAG)
    {
      main_screen_data.active_main_tile =
          (main_screen_data.active_main_tile + 4) % 5 +
          (5 * (main_screen_data.active_main_tile / 5));
    }
  else if (pending_flag == DOWN_FLAG)
    {
      main_screen_data.active_main_tile =
          (main_screen_data.active_main_tile + 1) % 5 +
          (5 * (main_screen_data.active_main_tile / 5));
    }

  return;
}

// change cursor position on screen
static void mm_change_tile_cursor_pos(buttons_state_t pending_flag)
{
  // erase active tile
  draw_mm_cursor(HMI_BACKGROUND_COLOR, main_screen_data.active_main_tile);
  mm_update_tile_cursor_pos(pending_flag);
  // draw new active tile
  draw_mm_cursor(HMI_CURSOR_COLOR, main_screen_data.active_main_tile);
  buttons_reset_flag(pending_flag);

  return;
}

static hmi_change_screen_t mm_check_pending_flags(void)
{
  buttons_state_t pending_flag = buttons_get_pending_flag();

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

static void main_menu_active(void)
{
  while (1)
    {
      // do every tile callback
      for (uint8_t i = 0; i < 10; i++)
        {
          if (NULL != main_screen_data.buttons[i].callback)
            {
              main_screen_data.buttons[i].callback(
                  &main_screen_data.buttons[i].data);
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
  main_screen_data.active_main_tile = 0;
  draw_main_screen(main_screen_data.active_main_tile);
  hmi_state = MAIN_MENU;
  return;
}

static void init_edit_menu(void)
{
  em_open_edit_menu(&main_screen_data);
  hmi_state = EDIT_MENU;
  return;
}

static void edit_menu_active(void)
{
  if (OPEN_MAIN_MENU == em_active_screen(&main_screen_data))
    {
      hmi_state = INIT_MAIN_MENU;
    }

  return;
}

void hmi_read_tile_function(const struct tile_data *p_data)
{
  // read on DMA
  frame_returned = false;
  bool timeout_error = false;
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)p_data, MAX_FRAME_SIZE);

  // send read command
  xgb_read_single_device(p_data->device_type, p_data->size_mark, p_data->address);

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
