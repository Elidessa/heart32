#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include "esp_lcd_types.h"
#include "types.h"

esp_lcd_panel_handle_t lcd_panel_setup();

void set_pixel(int x, int y, uint8_t* canvas);

void draw_square(int x,int y, int size, uint8_t *canvas);

void draw_figure_on_canvas(uint8_t* canvas, character* fig);
#endif
