#ifndef LCD_H
#define LCD_H

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lcd_types.h"
#include "hal/lcd_types.h"
#include "soc/gpio_num.h"
#include <stdint.h>
#include "symbols.h"
#include <string.h>

#ifdef R32
#define I2C_MASTER_SCL_IO 32
#define I2C_MASTER_SDA_IO 33
#else
#define I2C_MASTER_SCL_IO 32
#define I2C_MASTER_SDA_IO 33 
#endif

#define TEST_I2C_PORT -1
#define LCD_ADDR 0x3C

void animate_heart(esp_lcd_panel_handle_t panel_handle, uint8_t* canvas );
esp_lcd_panel_handle_t lcd_panel_setup();

void set_pixel(int x, int y, uint8_t *canvas);
void clear_pixel(int x, int y, uint8_t *canvas);
void draw_square(int x, int y, int size, uint8_t *canvas);

void draw_line(int x1, int y1, int x2, int y2, uint8_t *canvas);


void clear_figure_on_canvas(uint8_t *canvas, const character *fig);
void draw_figure_on_canvas(uint8_t* canvas, const character* fig);
#endif
