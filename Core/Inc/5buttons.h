/*
 * 5buttons.h
 *
 *  Created on: Apr 21, 2022
 *      Author: pawel
 */

#ifndef INC_5BUTTONS_H_
#define INC_5BUTTONS_H_

#include "stdbool.h"

typedef enum buttons_state
{
  IDLE,
  LEFT_FLAG,
  RIGHT_FLAG,
  DOWN_FLAG,
  UP_FLAG,
  ENTER_FLAG
} buttons_state_t;

typedef struct button_flags
{
  bool left_flag;
  bool right_flag;
  bool down_flag;
  bool up_flag;
  bool enter_flag;
} button_flags_t;

buttons_state_t buttons_check_flag(void);
void buttons_reset_flag(buttons_state_t state_flag);

#endif /* INC_5BUTTONS_H_ */
