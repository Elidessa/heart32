#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <stdint.h>
#include <string.h>

#include "font.h"
#include "lcd.h"
#include "sampler.h"
#include <esp_task_wdt.h>

#define BEATS_FOR_BPM 10

static int plot_buf[128] = {0};

void update_waveform(int val);
void calculate_bpm(int sample);
volatile float bpm;
volatile int prev_sample = 0;

volatile unsigned int beat_times[BEATS_FOR_BPM] = {0};
volatile int beat[BEATS_FOR_BPM] = {0};





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
			calculate_bpm(sample);
    }

    memset(canvas, 0, 512);

    for (int x = 0; x < 127; x++) {
      int y = scale_y(plot_buf[x]);
      int next_y = scale_y(plot_buf[x + 1]);
      draw_line(x, y, x + 1, next_y, canvas);
    }


		esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 128, 32, canvas);
		vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void update_waveform(int val) {
  memmove(plot_buf, plot_buf + 1, sizeof(int) * (128 - 1));
	plot_buf[127] = val;
}

void app_main(void) {

  sampler_init();

  esp_lcd_panel_handle_t panel_handle = lcd_panel_setup();

  xTaskCreate(display_task, "display_task", 4096, (void *)panel_handle, 5,
              NULL);
}
void calculate_bpm(int sample){
		int threshold = -1200;
		//calc BPM
		unsigned int first_beat = esp_timer_get_time();
		unsigned int last_beat = 0;
		int beats = 0;

		for(int i = 0; i < BEATS_FOR_BPM; i++){
				if(beat[i]){
						beats++;

						if(beat_times[i] < first_beat) first_beat = beat_times[i];
						if(beat_times[i] > last_beat) last_beat = beat_times[i];
				}
		}

		if(1){
				int temp = beats;
				bpm = (temp/((float)(last_beat - first_beat)/1000000))*60;
				printf("BPM: %f, BEATS: %d, TIME: %d\n", bpm, beats,(int)((last_beat - first_beat)/1000000));
		}else bpm = 0;

		
		//Beat detected, time and beat added to array
		if(sample < threshold && prev_sample > threshold ){
				for(int i = BEATS_FOR_BPM - 1; i > 0; i--){
						beat_times[i] = beat_times[i - 1];
						beat[i] = beat[i - 1];
				}
				beat_times[0] = esp_timer_get_time();
				beat[0] = 1;
		}
		prev_sample = sample;
		
		//beat not detected for 2 seconds, remove one beat. 
		
		if((esp_timer_get_time() - beat_times[0]) > 2000000 && beats){
				printf("remove beat\n");
				for(int i = BEATS_FOR_BPM - 1; i > 0; i--){
						beat_times[i] = beat_times[i - 1];
						beat[i] = beat[i - 1];
				}
				beat_times[0] = esp_timer_get_time();
				beat[0] = 0;
		}
}
