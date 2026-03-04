#include <stdint.h>
#include <string.h>
#include "esp_lcd_types.h"
#include "esp_lcd_panel_ops.h"
#include "lcd.h"

void draw_figure(uint8_t* canvas);
const uint8_t test[] = {
				1,1,0,1,1,
				1,1,0,1,1,
				0,0,0,0,0,
				0,0,0,0,0,
				1,0,0,0,1,
				0,1,1,1,0
		};


void app_main(void){
		const uint8_t bmp[] = {
				0xf0,0x29,0x29,0xf0};

		
		uint8_t *canvas = malloc(128*32/8);
		memset(canvas, 0, 512);

		esp_lcd_panel_handle_t panel_handle = lcd_panel_setup();	
		esp_lcd_panel_disp_on_off(panel_handle, true);
		
		//draw_square(9, 9, 10, canvas);
		draw_figure(canvas);

		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0,
										128, 32 , canvas);

		//esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 4, 8, bmp);

}
void draw_figure(uint8_t* canvas){
		int fig_x = 5;
		int fig_y = 6;

		for(int i = 0; i < fig_x*fig_y; i++){
				static int y = 0;
				if(test[i])set_pixel(i%fig_x,y,canvas);
				if(i%fig_x == fig_x-1)y++;
		}
}

