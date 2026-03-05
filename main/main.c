#include <stdint.h>
#include <string.h>
#include "esp_lcd_types.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/projdefs.h"


#define R32
#include "lcd.h"
#include <esp_task_wdt.h>
#include "types.h"

character heart1 = {
		.x = 9,
		.y = 9,
		.elements = {
				0,1,1,0,0,0,1,1,0,
				1,1,0,1,0,1,1,0,1,
				1,1,1,0,1,1,1,0,1,
				1,1,1,1,1,1,1,0,1,
				0,1,1,1,1,1,0,1,0,
				0,0,1,1,1,0,1,0,0,
				0,0,0,1,0,1,0,0,0,
				0,0,0,0,1,0,0,0,0,
				0,0,0,0,0,0,0,0,0
		}
};
character heart2 = {
		.x = 9,
		.y = 9,
		.elements = {
				0,0,0,0,0,0,0,0,0,
				0,0,1,0,0,0,1,0,0,
				0,1,1,1,0,1,1,1,0,
				0,1,1,1,1,1,0,1,0,
				0,0,1,1,1,0,1,0,0,
				0,0,0,1,0,1,0,0,0,
				0,0,0,0,1,0,0,0,0,
				0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0
		}
};
character face = {
		.x = 9,
		.y = 9,
		.elements = {
				0,1,1,0,0,0,1,1,0,
				1,1,0,1,0,1,1,0,1,
				0,1,1,0,0,0,1,1,0,
				0,0,0,0,0,0,0,0,0,
				0,0,0,0,1,0,0,0,0,
				0,0,0,1,1,0,0,0,0,
				1,1,0,0,0,0,0,1,1,
				0,1,1,1,1,1,1,1,0,
				0,0,1,1,1,1,1,0,0
		}
};

void draw_figure(uint8_t* canvas);
const uint8_t test[] = {
				1,1,0,1,1,
				1,1,0,1,1,
				0,0,0,0,0,
				0,0,0,0,0,
				1,0,0,0,1,
				0,1,1,1,0
		};

void animate_heart(esp_lcd_panel_handle_t panel_handle, uint8_t* canvas );

void app_main(void){
		uint8_t *canvas = malloc(128*32/8);
		memset(canvas, 0, 512);

		esp_lcd_panel_handle_t panel_handle = lcd_panel_setup();	

		draw_line(100, 0, 50, 30, canvas);
		
		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 128, 32, canvas);

		/*
		while(true){
				animate_heart(panel_handle, canvas);
		}
		*/

}
void animate_heart(esp_lcd_panel_handle_t panel_handle, uint8_t* canvas ){
		memset(canvas, 0, 512);
		draw_figure_on_canvas(canvas, &heart1);

		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0,
						128, 32 , canvas);

		vTaskDelay(pdMS_TO_TICKS(500));
		memset(canvas, 0, 512);

		draw_figure_on_canvas(canvas, &heart2);


		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0,
						128, 32 , canvas);
		vTaskDelay(pdMS_TO_TICKS(100));
}

