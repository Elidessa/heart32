#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_types.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lcd_panel_ops.h"
#include "hal/lcd_types.h"
#include "soc/gpio_num.h"
#include "lcd.h"

#define I2C_MASTER_SCL_IO 32
#define I2C_MASTER_SDA_IO 33
#define TEST_I2C_PORT -1
#define LCD_ADDR 0x3C

esp_lcd_panel_handle_t lcd_panel_setup(){
		i2c_master_bus_config_t i2c_mst_config = {
				.clk_source = I2C_CLK_SRC_DEFAULT,
				.i2c_port = TEST_I2C_PORT,
				.scl_io_num = I2C_MASTER_SCL_IO,
				.sda_io_num = I2C_MASTER_SDA_IO,
				.glitch_ignore_cnt = 4,
				.flags.enable_internal_pullup = true,
				.trans_queue_depth = 0,
				.flags.allow_pd = false,
		};

		i2c_master_bus_handle_t bus_handle;
		ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

		esp_lcd_panel_io_handle_t io_handle = NULL;
		esp_lcd_panel_io_i2c_config_t io_config = {
				.dev_addr = LCD_ADDR,
				.scl_speed_hz = 400000,
				.control_phase_bytes = 1,
				.dc_bit_offset = 6,
				.lcd_cmd_bits = 8,
				.lcd_param_bits = 8,
				.on_color_trans_done = NULL,
				.user_ctx = NULL,
				.flags = {
						.dc_low_on_data = false,
						.disable_control_phase = false,
				}

		};

		ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &io_config, &io_handle));

		esp_lcd_panel_ssd1306_config_t ssd1306_c = {
				.height = 32,
		};

		esp_lcd_panel_handle_t panel_handle = NULL;
		esp_lcd_panel_dev_config_t panel_config = {
				.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
				.data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
				.bits_per_pixel = 1,
				.reset_gpio_num = GPIO_NUM_NC,
				.vendor_config = &ssd1306_c,
				.flags = {
						.reset_active_high = false,
				}
		};
		ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle,&panel_config,&panel_handle));

		esp_lcd_panel_reset(panel_handle);
		esp_lcd_panel_init(panel_handle);

		return panel_handle;

}
void set_pixel(int x, int y, uint8_t* canvas){
		int index = (y/8)*128+x;
		canvas[index] |= (1<<(y%8));
}
void draw_square(int x,int y, int size, uint8_t *canvas){
		if(size+x > 128 || size+y > 32 || x<0 || y<0)return;

		for(int i = x; i < size+x; i++){
				for(int j = y; j < size+y; j++){
						set_pixel(i,j,canvas);
				}
		}
}
