#include "lcd.h"
#include <stdint.h>
#include <esp_task_wdt.h>
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_dev.h"
#include "hal/lcd_types.h"
#include "soc/gpio_num.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "freertos/projdefs.h"
#include "symbols.h"
#include <string.h>

esp_lcd_panel_handle_t lcd_panel_setup() {
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
      .flags =
          {
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
      }};
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));

  esp_lcd_panel_reset(panel_handle);
  esp_lcd_panel_init(panel_handle);

  esp_lcd_panel_disp_on_off(panel_handle, true);
  return panel_handle;
}

void set_pixel(int x, int y, uint8_t *canvas) {
  int index = (y / 8) * 128 + x;
  canvas[index] |= (1 << (y % 8));
}
void clear_pixel(int x, int y, uint8_t *canvas){
  int index = (y / 8) * 128 + x;
  canvas[index] &= (0 << (y % 8));
}

void draw_square(int x, int y, int size, uint8_t *canvas) {
  if (size + x > 128 || size + y > 32 || x < 0 || y < 0)
    return;

  for (int i = x; i < size + x; i++) {
    for (int j = y; j < size + y; j++) {
      set_pixel(i, j, canvas);
    }
  }
}

void draw_line(int x1, int y1, int x2, int y2, uint8_t *canvas) {
  uint8_t tmp;
  uint8_t x, y;
  uint8_t dx, dy;
  int8_t err;
  int8_t ystep;

  uint8_t swapxy = 0;

  if (x1 > x2)
    dx = x1 - x2;
  else
    dx = x2 - x1;
  if (y1 > y2)
    dy = y1 - y2;
  else
    dy = y2 - y1;

  if (dy > dx) {
    swapxy = 1;
    tmp = dx;
    dx = dy;
    dy = tmp;
    tmp = x1;
    x1 = y1;
    y1 = tmp;
    tmp = x2;
    x2 = y2;
    y2 = tmp;
  }

  if (x1 > x2) {
    tmp = x1;
    x1 = x2;
    x2 = tmp;
    tmp = y1;
    y1 = y2;
    y2 = tmp;
  }
  err = dx >> 1;
  if (y2 > y1)
    ystep = 1;
  else
    ystep = -1;
  y = y1;

  if (x2 == 255)
    x2--;

  for (x = x1; x <= x2; x++) {
    if (swapxy == 0)
      set_pixel(x, y, canvas);
    else
      set_pixel(y, x, canvas);
    err -= (uint8_t)dy;
    if (err < 0) {
      y += (uint8_t)ystep;
      err += (uint8_t)dx;
    }
  }
}

void draw_figure_on_canvas(uint8_t *canvas, const character *fig) {
  int fig_x = fig->x;
  int fig_y = fig->y;

  int y = 0;
  for (int i = 0; i < fig_x * fig_y; i++) {
    if (fig->elements[i])
      set_pixel(i % fig_x, y, canvas);
    if (i % fig_x == fig_x - 1)
      y++;
  }
}
void clear_figure_on_canvas(uint8_t *canvas, const character *fig) {
  int fig_x = fig->x;
  int fig_y = fig->y;

  int y = 0;
  for (int i = 0; i < fig_x * fig_y; i++) {
    if (fig->elements[i])
      clear_pixel(i % fig_x, y, canvas);
    if (i % fig_x == fig_x - 1)
      y++;
  }
}

void animate_heart(esp_lcd_panel_handle_t panel_handle, uint8_t* canvas ){
		clear_figure_on_canvas(canvas, &heart2);
		draw_figure_on_canvas(canvas, &heart1);

		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0,
						128, 32 , canvas);

		vTaskDelay(pdMS_TO_TICKS(500));
		clear_figure_on_canvas(canvas, &heart1);

		draw_figure_on_canvas(canvas, &heart2);


		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0,
						128, 32 , canvas);
		vTaskDelay(pdMS_TO_TICKS(100));
}
