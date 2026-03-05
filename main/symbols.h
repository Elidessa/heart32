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
extern character face;
extern character char_0;
extern character char_1;
extern character char_2;
extern character char_3;
extern character char_4;
extern character char_5;
extern character char_6;
extern character char_7;
extern character char_8;
extern character char_9;

