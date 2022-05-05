#ifndef INC_HMI_DRAW_H_
#define INC_HMI_DRAW_H_

#include "ILI9341.h"
#include "GFX_COLOR.h"

// Colors
#define HMI_TILE_COLOR ILI9341_YELLOW
#define HMI_BACKGROUND_COLOR ILI9341_BLACK
#define HMI_TEXT_COLOR ILI9341_YELLOW
#define HMI_CURSOR_COLOR ILI9341_DARKCYAN
#define HMI_EDIT_MENU_COLOR ILI9341_DARKCYAN
#define HMI_HIGHLIGHT_TILE_COLOR ILI9341_RED

void draw_small_tile(const uint8_t tile_number);
void draw_wide_tile(const char *text, uint8_t tile_number, bool center_text,
                    ColorType color);
void draw_mm_cursor(ColorType color, uint8_t active_tile);
void draw_main_screen(uint8_t active_tile);

// edit menu draw
void draw_edit_menu(uint8_t active_main_tile);
void draw_arrows_icon(ColorType color);
void draw_address_char(const hmi_edit_cursors_t *p_cursors);
void draw_exit_cursor(const hmi_edit_cursors_t *p_cursors, ColorType color);
void draw_address_cursor(const hmi_edit_cursors_t *p_cursors, ColorType color);
void draw_update_tile_number(char number);
void draw_erase_std_switch_txt(const hmi_edit_cursors_t *p_cursors, const edit_option_t **p_std_switch);
void draw_std_switch_txt(const hmi_edit_cursors_t *p_cursors,
                         uint8_t switch_number,const edit_option_t **p_std_switch);
void draw_cursor_initial_values(const hmi_edit_cursors_t *p_cursors,const edit_option_t **p_std_switch);


#endif // (INC_HMI_DRAW_H_)
