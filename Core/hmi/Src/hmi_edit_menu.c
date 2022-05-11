/*
 * hmi_edit_menu.c
 *
 *  Created on: 4 maj 2022
 *      Author: ROJEK
 */

#include "string.h"

#include "5buttons.h"

#include "hmi.h"
#include "hmi_draw.h"
#include "hmi_edit_menu.h"

#define CONFIRM 0U
#define DISCARD 1U

/* possible chars 0-9 */
#define NO_ADDRESS_CHARS 10U
#define NO_EDIT_MENU_TILES 6U

#define NO_OPTIONS_FUNCTION (sizeof(fun_switch))

static hmi_edit_cursors_t edit_menu_cursors = {0};

static const edit_option_t fun_switch[] = {{"<READ>", READ},
                                           {"<WRITE_CONT>", WRITE_CONT},
                                           {"<WRTIE_SINGLE>", WRITE_SINGLE}};

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

/* Standard switches are in places 1,2,3 so i fill position 0 with null switch*/
static const edit_option_t null_switch[] = {};

static const edit_option_t *std_switches[] = {null_switch, fun_switch,
                                              device_switch, size_switch};

static void init_edit_menu_cursors(void);
static hmi_change_screen_t
edit_menu_if_button_pressed(hmi_main_screen_t *p_main_screen_data);
static void save_data_to_tile(hmi_main_screen_t *p_main_screen_data);

void em_open_edit_menu(const hmi_main_screen_t *p_main_screen_data)
{
  draw_edit_menu(p_main_screen_data->active_main_tile);
  init_edit_menu_cursors();
  draw_cursors_initial_values(&edit_menu_cursors, std_switches);

  return;
}

/* Active screen super loop */
hmi_change_screen_t em_active_screen(hmi_main_screen_t *p_main_screen_data)
{

  hmi_change_screen_t ret_action = OPEN_MAIN_MENU;

  while (1)
    {
      ret_action = edit_menu_if_button_pressed(p_main_screen_data);

      if (NO_CHANGE != ret_action)
        {
          if (SAVE_DATA_TO_TILE == ret_action)
            {
              save_data_to_tile(p_main_screen_data);
              ret_action = OPEN_MAIN_MENU;
            }
          break;
        }
    }

  return ret_action;
}

/*** HORIZONTAL CURSOR FUNCTIONS **/

static void update_horiz_cursor_val(buttons_state_t pending_flag)
{
  if (pending_flag == LEFT_FLAG)
    {
      switch (edit_menu_cursors.vert_tile)
        {
        case (TILE_FUNCTION):
          edit_menu_cursors.horiz_fun = (edit_menu_cursors.horiz_fun + 1) % 2;
          break;
        case (TILE_DEVICE):
          edit_menu_cursors.horiz_dev = (edit_menu_cursors.horiz_dev + 11) % 12;
          break;
        case (TILE_SIZE):
          edit_menu_cursors.horiz_size = (edit_menu_cursors.horiz_size + 4) % 5;
          break;
        case (TILE_ADDRESS):
          edit_menu_cursors.horiz_address =
              (edit_menu_cursors.horiz_address + 5) % 6;
          break;
        case (TILE_EXIT):
          edit_menu_cursors.horiz_exit = (edit_menu_cursors.horiz_exit + 1) % 2;
          break;
        case (TILE_HEADER):
          /* FALLTHORUGH */
        default:
          break;
        }
    }
  else if (pending_flag == RIGHT_FLAG)
    {
      switch (edit_menu_cursors.vert_tile)
        {
        case (TILE_FUNCTION):
          edit_menu_cursors.horiz_fun = (edit_menu_cursors.horiz_fun + 1) % 2;
          break;
        case (TILE_DEVICE):
          edit_menu_cursors.horiz_dev = (edit_menu_cursors.horiz_dev + 1) % 12;
          break;
        case (TILE_SIZE):
          edit_menu_cursors.horiz_size = (edit_menu_cursors.horiz_size + 1) % 5;
          break;
        case (TILE_ADDRESS):
          edit_menu_cursors.horiz_address =
              (edit_menu_cursors.horiz_address + 1) % 6;
          break;
        case (TILE_EXIT):
          edit_menu_cursors.horiz_exit = (edit_menu_cursors.horiz_exit + 1) % 2;
          break;
        case (TILE_HEADER):
          /* FALLTHORUGH */
        default:
          break;
        }
    }

  return;
}

/* Standard switches : function,device,size */
static void redraw_horiz_std_switch(buttons_state_t pending_flag)
{
  draw_erase_std_switch_text(&edit_menu_cursors, std_switches);
  update_horiz_cursor_val(pending_flag);
  draw_std_switch_text(&edit_menu_cursors, edit_menu_cursors.vert_tile,
                       std_switches);
  return;
}

static void redraw_horiz_address_switch(buttons_state_t pending_flag)
{
  draw_address_cursor(&edit_menu_cursors, HMI_EDIT_MENU_COLOR);
  update_horiz_cursor_val(pending_flag);

  /* Save char to main menu data */
  edit_menu_cursors.vert_address_num =
      (uint8_t)edit_menu_cursors.address[edit_menu_cursors.horiz_address];

  if (true == edit_menu_cursors.is_edit_mode_active)
    {
      draw_address_cursor(&edit_menu_cursors, HMI_HIGHLIGHT_TILE_COLOR);
    }
  else
    {
      draw_address_cursor(&edit_menu_cursors, HMI_TEXT_COLOR);
    }

  return;
}

static void redraw_horiz_exit_switch(buttons_state_t pending_flag)
{
  draw_exit_cursor(&edit_menu_cursors, HMI_EDIT_MENU_COLOR);
  update_horiz_cursor_val(pending_flag);
  draw_exit_cursor(&edit_menu_cursors, HMI_TEXT_COLOR);

  return;
}

static void redraw_horiz_header(buttons_state_t pending_flag,
                                hmi_main_screen_t *p_main_screen_data)
{
  if (RIGHT_FLAG == pending_flag)
    {
      p_main_screen_data->active_main_tile =
          (p_main_screen_data->active_main_tile + 1) % 10;
    }
  else if (LEFT_FLAG == pending_flag)
    {
      p_main_screen_data->active_main_tile =
          (p_main_screen_data->active_main_tile + 9) % 10;
    }

  draw_update_header_number(p_main_screen_data->active_main_tile + '0');

  return;
}

static void shift_vert_cursor_up(cursor *p_cursor_to_update,
                                 uint8_t no_positions)
{
  cursor cursor_value = *p_cursor_to_update;
  cursor_value = (cursor_value + (no_positions - 1)) % no_positions;
  *p_cursor_to_update = cursor_value;
  return;
}

static void shift_vert_cursor_down(cursor *p_cursor_to_update,
                                   uint8_t no_positions)
{
  cursor cursor_value = *p_cursor_to_update;
  cursor_value = (cursor_value + 1) % no_positions;
  *p_cursor_to_update = cursor_value;
  return;
}

static bool is_edit_mode_active(void)
{
  return (TILE_ADDRESS == edit_menu_cursors.vert_tile &&
          true == edit_menu_cursors.is_edit_mode_active);
}

static void update_vert_cursor_val(buttons_state_t pending_flag)
{

  if (UP_FLAG == pending_flag)
    {
      if (true == is_edit_mode_active)
        {
          shift_vert_cursor_up(&edit_menu_cursors.vert_address_num,
                               NO_ADDRESS_CHARS);
        }
      else
        {
          shift_vert_cursor_up(&edit_menu_cursors.vert_tile,
                               NO_EDIT_MENU_TILES);
        }
    }
  else if (DOWN_FLAG == pending_flag)
    {
      if (true == is_edit_mode_active)
        {
          shift_vert_cursor_down(&edit_menu_cursors.vert_address_num,
                                 NO_ADDRESS_CHARS);
        }
      else
        {
          shift_vert_cursor_down(&edit_menu_cursors.vert_tile,
                                 NO_EDIT_MENU_TILES);
        }
    }

  return;
}

static void select_horiz_cursor_to_edit(buttons_state_t pending_flag,
                                        hmi_main_screen_t *p_main_screen_data)
{
  switch (edit_menu_cursors.vert_tile)
    {
    case (TILE_HEADER):
      redraw_horiz_header(pending_flag, p_main_screen_data);
      break;

    case (TILE_DEVICE):
      /* FALLTHORUGH */
    case (TILE_FUNCTION):
      /* FALLTHORUGH */
    case (TILE_SIZE):
      redraw_horiz_std_switch(pending_flag);
      break;

    case (TILE_ADDRESS):
      redraw_horiz_address_switch(pending_flag);
      break;

    case (TILE_EXIT):
      redraw_horiz_exit_switch(pending_flag);
      break;
    }

  buttons_reset_flag(pending_flag);

  return;
}

/*** VERTICAL CURSOR CHANGE FUNCTIONS **/

static void redraw_vert_address_char(buttons_state_t pending_flag)
{
  update_vert_cursor_val(pending_flag);
  draw_address_char(&edit_menu_cursors);
  edit_menu_cursors.address[edit_menu_cursors.horiz_address] =
      (char)edit_menu_cursors.vert_address_num;

  return;
}

// update and redraw vertical tile selection
static void redraw_vert_tile_cursor(buttons_state_t pending_flag)
{
  draw_wide_tile(NULL, edit_menu_cursors.vert_tile, false, HMI_TILE_COLOR);
  update_vert_cursor_val(pending_flag);
  draw_wide_tile(NULL, edit_menu_cursors.vert_tile, false,
                 HMI_HIGHLIGHT_TILE_COLOR);

  if (TILE_ADDRESS == edit_menu_cursors.vert_tile)
    {
      draw_address_cursor(&edit_menu_cursors, HMI_TEXT_COLOR);
    }
  else
    {
      draw_address_cursor(&edit_menu_cursors, HMI_EDIT_MENU_COLOR);
    }

  if (TILE_EXIT == edit_menu_cursors.vert_tile)
    {
      draw_exit_cursor(&edit_menu_cursors, HMI_TEXT_COLOR);
    }
  else
    {
      draw_exit_cursor(&edit_menu_cursors, HMI_EDIT_MENU_COLOR);
    }

  return;
}

// change tile cursor position on screen vertically
static void select_vert_cursor_to_edit(buttons_state_t pending_flag)
{

  if (TILE_ADDRESS == edit_menu_cursors.vert_tile &&
      true == edit_menu_cursors.is_edit_mode_active)
    {
      redraw_vert_address_char(pending_flag);
    }
  else
    {
      redraw_vert_tile_cursor(pending_flag);
    }

  buttons_reset_flag(pending_flag);

  return;
}

/*** ENTER PRESSED FUNCTIONS **/

static void show_arrows_icon_if_edit_active(bool edit_mode_active)
{
  if (true == edit_mode_active)
    {
      draw_arrows_icon(HMI_TEXT_COLOR);
      draw_address_cursor(&edit_menu_cursors, HMI_HIGHLIGHT_TILE_COLOR);
    }
  else
    {
      draw_arrows_icon(HMI_EDIT_MENU_COLOR);
      draw_address_cursor(&edit_menu_cursors, HMI_TEXT_COLOR);
    }

  return;
}

static hmi_change_screen_t action_if_enter_pressed(buttons_state_t pending_flag)
{

  hmi_change_screen_t ret_action = NO_CHANGE;

  switch (edit_menu_cursors.vert_tile)
    {
    case (TILE_ADDRESS):
      {
        edit_menu_cursors.is_edit_mode_active =
            !(edit_menu_cursors.is_edit_mode_active);

        show_arrows_icon_if_edit_active(edit_menu_cursors.is_edit_mode_active);
        break;
      }
    case (TILE_EXIT):
      {
        if (edit_menu_cursors.horiz_exit == CONFIRM)
          {
            ret_action = SAVE_DATA_TO_TILE;
          }
        else if (edit_menu_cursors.horiz_exit == DISCARD)
          {
            ret_action = OPEN_MAIN_MENU;
          }
        break;
      }

    case (TILE_HEADER):
      /* FALLTHORUGH */
    case (TILE_DEVICE):
      /* FALLTHORUGH */
    case (TILE_SIZE):
      /* FALLTHORUGH */
    case (TILE_FUNCTION):
      /* FALLTHORUGH */
    default:
      break;
    }

  buttons_reset_flag(pending_flag);

  return ret_action;
}

static tile_callback_t get_callback_to_tile(tile_function_t tile_function)
{
  tile_callback_t ret_ptr = NULL;

  switch (tile_function)
    {
    case (READ):
      {
        ret_ptr = &hmi_read_tile_function;
        break;
      }
    case (WRITE_CONT):
      {
        ret_ptr = NULL;
        break;
      }
    case (WRITE_SINGLE):
      {
        ret_ptr = NULL;
        break;
      }
    default:
      break;
    }

  return ret_ptr;
}

static void init_edit_menu_cursors(void)
{
  memcpy(&edit_menu_cursors.address, "00000", 6);
  edit_menu_cursors.is_edit_mode_active = false;
  edit_menu_cursors.horiz_address = 0;
  edit_menu_cursors.horiz_dev = 0;
  edit_menu_cursors.horiz_exit = 0;
  edit_menu_cursors.horiz_fun = 0;
  edit_menu_cursors.horiz_size = 0;
  edit_menu_cursors.vert_address_num = 0;
  edit_menu_cursors.vert_tile = TILE_HEADER;

  return;
}

static hmi_change_screen_t
edit_menu_if_button_pressed(hmi_main_screen_t *p_main_screen_data)
{
  hmi_change_screen_t ret_action = NO_CHANGE;
  buttons_state_t pending_flag = buttons_get_pending_flag();

  if (IDLE != pending_flag)
    {
      switch (pending_flag)
        {
        case (LEFT_FLAG):
          /* FALLTHORUGH */
        case (RIGHT_FLAG):
          select_horiz_cursor_to_edit(pending_flag, p_main_screen_data);
          break;
        case (UP_FLAG):
          /* FALLTHORUGH */
        case (DOWN_FLAG):
          select_vert_cursor_to_edit(pending_flag);
          break;
        case (ENTER_FLAG):
          (ret_action = action_if_enter_pressed(pending_flag));
          break;
        case (IDLE):
          /* FALLTHORUGH */
        default:
          break;
        }
    }

  return ret_action;
}

static void save_data_to_tile(hmi_main_screen_t *p_main_screen_data)
{
  uint8_t save_tile_number = p_main_screen_data->active_main_tile;
  uint8_t save_device = edit_menu_cursors.horiz_dev;
  uint8_t save_size = edit_menu_cursors.horiz_size;
  uint8_t save_function = edit_menu_cursors.horiz_fun;

  /* Copy all the significant data*/
  p_main_screen_data->buttons[save_tile_number].data.tile_number =
      save_tile_number;
  p_main_screen_data->buttons[save_tile_number].data.device_type =
      device_switch[save_device].frame_letter;
  p_main_screen_data->buttons[save_tile_number].data.size_mark =
      size_switch[save_size].frame_letter;
  p_main_screen_data->buttons[save_tile_number].data.function =
      fun_switch[save_function].frame_letter;
  memcpy(p_main_screen_data->buttons[save_tile_number].data.address,
         &(edit_menu_cursors.address), 6);

  p_main_screen_data->buttons[save_tile_number].callback =
      get_callback_to_tile(save_function);

  return;
}
