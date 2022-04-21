/*
 * 5buttons.c
 *
 *  Created on: Apr 21, 2022
 *      Author: pawel
 */

#include "5buttons.h"

#include "main.h"

#define BTN_LEFT BUTTON_LEFT_Pin
#define BTN_RIGHT BUTTON_RIGHT_Pin
#define BTN_DOWN BUTTON_DOWN_Pin
#define BTN_UP BUTTON_UP_Pin
#define BTN_ENTER BUTTON_ENTER_Pin

volatile button_flags_t flags;

void set_button_flag(uint16_t GPIO_Pin) {
  switch (GPIO_Pin) {
    case (BTN_LEFT):
      flags.left_flag = true;
      break;

    case (BTN_RIGHT):
      flags.right_flag = true;
      break;

    case (BTN_DOWN):
      flags.down_flag = true;
      break;

    case (BTN_UP):
      flags.up_flag = true;
      break;

    case (BTN_ENTER):
      flags.enter_flag = true;
      break;

    default:
      break;
      // different gpio
  }

  return;
}

buttons_state_t buttons_check_flag(void) {
  buttons_state_t active_button = IDLE;

  if (flags.left_flag) {
    active_button = LEFT_FLAG;
  } else if (flags.right_flag) {
    active_button = RIGHT_FLAG;
  } else if (flags.down_flag) {
    active_button = DOWN_FLAG;
  } else if (flags.up_flag) {
    active_button = UP_FLAG;
  } else if (flags.enter_flag) {
    active_button = ENTER_FLAG;
  }

  return active_button;
}

void buttons_reset_flag(buttons_state_t state_flag) {
  switch (state_flag) {
    case (LEFT_FLAG):
      flags.left_flag = false;
      break;

    case (RIGHT_FLAG):
      flags.right_flag = false;
      break;

    case (DOWN_FLAG):
      flags.down_flag = false;
      break;

    case (UP_FLAG):
      flags.up_flag = false;
      break;

    case (ENTER_FLAG):
      flags.enter_flag = false;
      break;

    case (IDLE):
    default:
      break;
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  set_button_flag(GPIO_Pin);
  return;
}
