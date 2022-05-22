/*
 * hmi.c
 *
 *  Created on: 15 kwi 2022
 *      Author: ROJEK
 */

#include "main.h"

#include "fonts.h"

#include "hmi.h"
#include "hmi_draw.h"
#include "hmi_edit_menu.h"
#include "hmi_main_menu.h"
#include "hmi_mock.h"

extern SPI_HandleTypeDef hspi1;
static volatile hmi_state_t hmi_state;

static void change_state(hmi_state_t state);

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

static void init_read_eeprom(void) { hmi_state = INIT_TFT; }

static void init_tft(void)
{
  ILI9341_Init(&hspi1);
  GFX_SetFont(font_8x5);
  change_state(INIT_MAIN_MENU);
  return;
}

static void init_main_menu(void)
{
  mm_write_initial_values_to_tiles();
  draw_main_screen(0);
  change_state(MAIN_MENU);
  return;
}

static void main_menu_active(void)
{

  if (OPEN_EDIT_MENU == mm_active_screen())
    {
      change_state(INIT_EDIT_MENU);
    }
  return;
}

static void init_edit_menu(void)
{
  em_open_edit_menu();
  change_state(EDIT_MENU);
  return;
}

static void edit_menu_active(void)
{
  if (OPEN_MAIN_MENU == em_active_screen())
    {
      change_state(INIT_MAIN_MENU);
    }
  return;
}

static void change_state(hmi_state_t state)
{
  hmi_state = state;
  return;
}
