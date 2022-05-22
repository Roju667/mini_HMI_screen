/*
 * hmi_main_menu.h
 *
 *  Created on: May 22, 2022
 *      Author: pawel
 */

#ifndef HMI_INC_HMI_MAIN_MENU_H_
#define HMI_INC_HMI_MAIN_MENU_H_

#define SWITCH_SCREEN 1U

#define RETURN_FRAME_TIMEOUT 50U

#define TIMEOUT_VAL (int32_t)0xFFFFFFFF
#define NAK_VAL (int32_t)0xFFFFFFFD
#define INITIAL_VAL (int32_t)0xFFFFFFFE

void mm_write_initial_values_to_tiles(void);
hmi_change_screen_t mm_active_screen(void);
void mm_read_tile_function(const struct frame_data *frame_send);

#endif /* HMI_INC_HMI_MAIN_MENU_H_ */
