#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <stdint.h>
#include <string.h>

#include "font.h"
#include "lcd.h"
#include "sampler.h"
#include <esp_task_wdt.h>

static int plot_buf[128] = {0};

void update_waveform(int val);

int scale_y(int val) {
  static int min = 4095, max = 0;
  for (int i = 0; i < 128; i++) {
    if (plot_buf[i] < min)
      min = plot_buf[i];
    if (plot_buf[i] > max)
      max = plot_buf[i];
  }
  int range = (max - min > 10) ? (max - min) : 10;
  return 31 - ((val - min) * 31 / range);
}

void display_task(void *pvParameters) {
  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)pvParameters;
  uint8_t *canvas = malloc(128 * 32 / 8);
  memset(canvas, 0, 512);
  int sample;

  while (1) {
    while (xQueueReceive(sample_queue, &sample, 0) == pdTRUE) {
      update_waveform(sample);
    }

    memset(canvas, 0, 512);

    for (int x = 0; x < 127; x++) {
      int y = scale_y(plot_buf[x]);
      int next_y = scale_y(plot_buf[x + 1]);
      draw_line(x, y, x + 1, next_y, canvas);
    }

<<<<<<< HEAD
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 128, 32, canvas);
    vTaskDelay(pdMS_TO_TICKS(50));
=======
		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 128, 32, canvas);
		vTaskDelay(pdMS_TO_TICKS(10));
>>>>>>> 1013b7c (medel raw)
  }
}

void update_waveform(int val) {
  memmove(plot_buf, plot_buf + 1, sizeof(int) * (128 - 1));
}

void app_main(void) {

  sampler_init();

  esp_lcd_panel_handle_t panel_handle = lcd_panel_setup();

  xTaskCreate(display_task, "display_task", 4096, (void *)panel_handle, 5,
              NULL);
}
