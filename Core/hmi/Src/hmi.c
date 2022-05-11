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
#include "hmi_mock.h"

#define SWITCH_SCREEN 1U

#define RETURN_FRAME_TIMEOUT 50U

#define TIMEOUT_VAL (int32_t)0xFFFFFFFF
#define INITIAL_VAL (int32_t)0xFFFFFFFE

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
static volatile hmi_state_t hmi_state;
static volatile bool frame_returned;
static hmi_main_screen_t main_screen_data;

static void update_main_cursor_val(buttons_state_t pending_flag);
static void redraw_main_cursor(buttons_state_t pending_flag);
static hmi_change_screen_t edit_screen_if_button_pressed(void);

static bool wait_for_frame_until_timeout(void);
static bool is_new_text_neccessary(char *text_in_tile, const u_frame *new_frame,
                                   bool timeout, hmi_tile_t *p_tile);
static void call_tile_function(uint8_t tile_number);

static void init_read_eeprom(void);
static void init_tft(void);
static void init_main_menu(void);
static void main_menu_active(void);
static void init_edit_menu(void);
static void edit_menu_active(void);

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
          }
        }
    }

  return;
}

#if (HMI_MOCK_COMM_READ == 0U)
void hmi_read_tile_function(const struct frame_data *frame_send)
{
  frame_returned = false;
  bool timeout_error = false;
  char msg_to_print[16];
  u_frame *p_frame_received = {0};
  hmi_tile_t *p_edited_tile = &main_screen_data.tiles[frame_send->tile_number];

  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)p_frame_received,
                               MAX_FRAME_SIZE);

  xgb_read_single_device(frame_send->device_type, frame_send->size_mark,
                         frame_send->address);

  timeout_error = wait_for_frame_until_timeout();

  if (is_new_text_neccessary(msg_to_print, p_frame_received, timeout_error,
                             p_edited_tile))
    {
      draw_small_tile_text(frame_send->tile_number, msg_to_print, true);
    }

  return;
}
#endif /* (HMI_MOCK_COMM_READ == 0U) */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  frame_returned = true;
}

static void update_main_cursor_val(buttons_state_t pending_flag)
{
  switch (pending_flag)
    {
    case (LEFT_FLAG):
      {
        main_screen_data.active_main_tile =
            (main_screen_data.active_main_tile + 5) % 10;
        break;
      }
    case (RIGHT_FLAG):
      {
        main_screen_data.active_main_tile =
            (main_screen_data.active_main_tile + 5) % 10;
        break;
      }
    case (UP_FLAG):
      {
        main_screen_data.active_main_tile =
            (main_screen_data.active_main_tile + 4) % 5 +
            (5 * (main_screen_data.active_main_tile / 5));
        break;
      }
    case (DOWN_FLAG):
      {
        main_screen_data.active_main_tile =
            (main_screen_data.active_main_tile + 1) % 5 +
            (5 * (main_screen_data.active_main_tile / 5));
        break;
      }

    case (ENTER_FLAG):
      /* FALLTHROUGH */
    case (IDLE):
      /* FALLTHROUGH */
    default:
      break;
    }

  return;
}

static void redraw_main_cursor(buttons_state_t pending_flag)
{
  draw_main_menu_cursor(HMI_BACKGROUND_COLOR,
                        main_screen_data.active_main_tile);
  update_main_cursor_val(pending_flag);
  draw_main_menu_cursor(HMI_CURSOR_COLOR, main_screen_data.active_main_tile);

  return;
}

static hmi_change_screen_t edit_screen_if_button_pressed(void)
{
  buttons_state_t pending_flag = buttons_get_pending_flag();

  hmi_change_screen_t change_screen = NO_CHANGE;

  if (IDLE != pending_flag)
    {
      switch (pending_flag)
        {
        case (LEFT_FLAG):
          /* FALLTHROUGH */
        case (RIGHT_FLAG):
          /* FALLTHROUGH */
        case (UP_FLAG):
          /* FALLTHROUGH */
        case (DOWN_FLAG):
          redraw_main_cursor(pending_flag);
          break;

        case (ENTER_FLAG):
          change_screen = OPEN_EDIT_MENU;
          break;
        case (IDLE):
          /* FALLTHROUGH */
        default:
          break;
        }
    }

  buttons_reset_flag(pending_flag);
  return change_screen;
}

static void write_initial_values_to_tiles(void)
{

  for (uint8_t i = 0; i < 10; i++)
    {
      main_screen_data.tiles[i].value = INITIAL_VAL;
    }

  return;
}

static void main_menu_active(void)
{
  while (1)
    {

      hmi_change_screen_t ret_action = NO_CHANGE;
      for (uint8_t i = 0; i < 10; i++)
        {
          call_tile_function(i);
          ret_action = edit_screen_if_button_pressed();

          if (OPEN_EDIT_MENU == ret_action)
            {
              hmi_state = INIT_EDIT_MENU;
              return;
            }
        }
    }
}

static void init_read_eeprom(void) { hmi_state = INIT_TFT; }

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
  write_initial_values_to_tiles();
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

static bool wait_for_frame_until_timeout(void)
{
  uint32_t current_tick = HAL_GetTick();
  bool timeout_error = false;
  while (false == frame_returned)
    {
      if (HAL_GetTick() - current_tick > RETURN_FRAME_TIMEOUT)
        {
          timeout_error = true;
          break;
        }
    }

  return timeout_error;
}

static void call_tile_function(uint8_t tile_number)
{

  if (NULL != main_screen_data.tiles[tile_number].callback)
    {
      main_screen_data.tiles[tile_number].callback(
          &main_screen_data.tiles[tile_number].data);
    }

  return;
}

static int32_t parse_text_from_frame(char *new_text,
                                     const u_frame *p_frame_data, bool timeout)
{
  int32_t ret_value;

  if (true == timeout)
    {
      strcpy(new_text, "TIMEOUT");
      ret_value = TIMEOUT_VAL;
    }

  return ret_value;
}

static bool is_new_val_different(int32_t new_val, int32_t current_val)
{
  return ((new_val != current_val) || INITIAL_VAL == current_val);
}

static bool is_new_text_neccessary(char *text_in_tile, const u_frame *new_frame,
                                   bool timeout, hmi_tile_t *p_tile)
{

  // parse value
  char new_text[16] = {0};
  int32_t new_value = parse_text_from_frame(new_text, new_frame, timeout);
  int32_t current_value = p_tile->value;
  bool draw_new_text = false;
  // check if its different than one before
  if (true == is_new_val_different(new_value, current_value))
    {
      strcpy(text_in_tile, new_text);
      draw_new_text = true;
    }

  p_tile->value = new_value;
  return draw_new_text;
}
