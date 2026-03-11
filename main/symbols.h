#pragma once

#include <stdint.h>

typedef struct{
		int x,y;
		uint8_t elements[];
}character;

void lcd_set_text_on_canvas(uint8_t* canvas, char* text);
character* get_char_ptr(char character);

extern character heart1;
extern character heart2;
extern character heart2_15x15;
extern character heart1_15x15;
extern character face;
