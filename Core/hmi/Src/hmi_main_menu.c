/*
 * hmi_main_menu.c
 *
 *  Created on: May 22, 2022
 *      Author: pawel
 */

#include "main.h"
#include "stdio.h"
#include "string.h"

#include "5buttons.h"

#include "hmi.h"
#include "hmi_draw.h"
#include "hmi_main_menu.h"
#include "xgb_comm.h"

extern UART_HandleTypeDef huart1;
static volatile bool frame_returned;

static uint8_t update_main_cursor_val(buttons_state_t pending_flag,
                                      uint8_t active_tile);
static void redraw_main_cursor(buttons_state_t pending_flag,
                               hmi_main_screen_t *main_screen_data);
static hmi_change_screen_t
edit_screen_if_button_pressed(hmi_main_screen_t *main_screen_data);

static bool wait_for_frame_until_timeout(void);
static bool is_new_text_neccessary(char *text_in_tile, const u_frame *new_frame,
                                   bool timeout, hmi_tile_t *p_tile);
static void call_tile_function(uint8_t tile_number,
                               hmi_main_screen_t *main_screen_data);

hmi_change_screen_t mm_active_screen(hmi_main_screen_t *main_screen_data)
{
  hmi_change_screen_t ret_action = NO_CHANGE;

  while (1)
    {
      for (uint8_t i = 0; i < 10; i++)
        {
          call_tile_function(i, main_screen_data);
          ret_action = edit_screen_if_button_pressed(main_screen_data);

          if (NO_CHANGE != ret_action)
            {
              return ret_action;
            }
        }
    }
}

void write_initial_values_to_tiles(hmi_main_screen_t *main_screen_data)
{

  for (uint8_t i = 0; i < 10; i++)
    {
      main_screen_data->tiles[i].value = INITIAL_VAL;
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

  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)p_frame_received,
                               MAX_FRAME_SIZE);

  xgb_read_single_device(frame_send->device_type, frame_send->size_mark,
                         frame_send->address);

  timeout_error = wait_for_frame_until_timeout();

  draw_small_tile_text(frame_send->tile_number, msg_to_print, true);

  return;
}
#endif /* (HMI_MOCK_COMM_READ == 0U) */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  frame_returned = true;
}

static uint8_t update_main_cursor_val(buttons_state_t pending_flag,
                                      uint8_t active_tile)
{
  switch (pending_flag)
    {
    case (LEFT_FLAG):
      {
        active_tile = (active_tile + 5) % 10;
        break;
      }
    case (RIGHT_FLAG):
      {
        active_tile = (active_tile + 5) % 10;
        break;
      }
    case (UP_FLAG):
      {
        active_tile = (active_tile + 4) % 5 + (5 * (active_tile / 5));
        break;
      }
    case (DOWN_FLAG):
      {
        active_tile = (active_tile + 1) % 5 + (5 * (active_tile / 5));
        break;
      }

    case (ENTER_FLAG):
      /* FALLTHROUGH */
    case (IDLE):
      /* FALLTHROUGH */
    default:
      break;
    }

  return active_tile;
}

static void redraw_main_cursor(buttons_state_t pending_flag,
                               hmi_main_screen_t *main_screen_data)
{
  draw_main_menu_cursor(HMI_BACKGROUND_COLOR,
                        main_screen_data->active_main_tile);
  main_screen_data->active_main_tile =
      update_main_cursor_val(pending_flag, main_screen_data->active_main_tile);
  draw_main_menu_cursor(HMI_CURSOR_COLOR, main_screen_data->active_main_tile);

  return;
}

static hmi_change_screen_t
edit_screen_if_button_pressed(hmi_main_screen_t *main_screen_data)
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
          redraw_main_cursor(pending_flag, main_screen_data);
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

static void call_tile_function(uint8_t tile_number,
                               hmi_main_screen_t *main_screen_data)
{

  if (NULL != main_screen_data->tiles[tile_number].callback)
    {
      main_screen_data->tiles[tile_number].callback(
          &main_screen_data->tiles[tile_number].data);
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
  else if (p_frame_data->nak_frame.header_nak == XGB_CC_NAK)
    {
      strcpy(new_text, "NAK");
      ret_value = NAK_VAL;
    }
  else
    {
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
