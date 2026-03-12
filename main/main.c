#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "font.h"
#include "lcd.h"
#include "sampler.h"

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 32
#define CANVAS_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)

static int plot_buf[DISPLAY_WIDTH] = {0};

void update_waveform(int val);

static void get_waveform_range(int *out_min, int *out_range) {
  int min = plot_buf[0], max = plot_buf[0];
  for (int i = 1; i < DISPLAY_WIDTH; i++) {
    if (plot_buf[i] < min)
      min = plot_buf[i];
    if (plot_buf[i] > max)
      max = plot_buf[i];
  }
  *out_min = min;
  *out_range = (max - min > 10) ? (max - min) : 10;
}

static int scale_y(int val, int min, int range) {
  return (DISPLAY_HEIGHT - 1) - ((val - min) * (DISPLAY_HEIGHT - 1) / range);
}

void display_task(void *pvParameters) {
  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)pvParameters;
  uint8_t *canvas = malloc(CANVAS_SIZE);
  int sample;
  while (1) {
    while (xQueueReceive(sample_queue, &sample, 0) == pdTRUE) {
      update_waveform(sample);
    }

    memset(canvas, 0, CANVAS_SIZE);

    int min, range;
    get_waveform_range(&min, &range);

    for (int x = 0; x < DISPLAY_WIDTH-1; x++) {
      int y = scale_y(plot_buf[x], min, range);
      int next_y = scale_y(plot_buf[x + 1], min, range);
      draw_line(x, y, x + 1, next_y, canvas);
    }
    // text på skärmen
    char text[20];
    snprintf(text, sizeof(text), "BPM: %d", bpm);
    draw_text(text, 1, 0, canvas);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                              canvas);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void update_waveform(int val) {
  memmove(plot_buf, plot_buf + 1, sizeof(int) * (DISPLAY_WIDTH - 1));
  plot_buf[DISPLAY_WIDTH - 1] = val;
}

void app_main(void) {

  sampler_init();

  esp_lcd_panel_handle_t panel_handle = lcd_panel_setup();

  xTaskCreate(display_task, "display_task", 4096, (void *)panel_handle, 5,
              NULL);
}
