#include <stdint.h>
#include "freertos/projdefs.h"
#include <string.h>
#include "esp_lcd_types.h"
#include "esp_lcd_panel_ops.h"
#include "lcd.h"
#include "font.h"

void app_main(void){
		uint8_t *canvas = malloc(128*32/8);
		memset(canvas, 0, 512);

		esp_lcd_panel_handle_t panel_handle = lcd_panel_setup();	

		draw_line(1, 12, 51, 12, canvas);
		draw_text("ESP32", 2, 0, canvas);
		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 128, 32, canvas);
		
		while(true){
				animate_heart(panel_handle, canvas);
		}
}



