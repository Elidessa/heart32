#include "sampler.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "hal/adc_types.h"
#include "highpass.h"
#include "lowpass.h"
#include "portmacro.h"
#include <stdint.h>
#include <string.h>

#define BEATS_FOR_BPM 10
#define THRESHOLD_FLOOR 200
#define THRESHOLD_DECAY 0.99f
#define UPPER_RATIO 0.75f
#define LOWER_RATIO 0.50f
#define BEAT_TIMEOUT_MS 2000
#define SAMPLE_PERIOD_MS 20
#define MAX_INTERVAL_MS 3000

typedef enum { WAITING_FOR_BEAT, IN_BEAT } beat_state_t;

void detect_heartbeat(int sample);
volatile int bpm;

static unsigned int intervals[BEATS_FOR_BPM] = {0};
static uint8_t interval_index = 0;
static uint8_t valid_intervals = 0;
static int interval_sum_ms = 0;
static int time_since_last_beat = 0;

static adc_oneshot_unit_handle_t adc_handle;
QueueHandle_t sample_queue = NULL;
float highpass_filter(float value);
float lowpass_filter(float value);

static void register_beat(void) {
  if (time_since_last_beat > MAX_INTERVAL_MS) {
    time_since_last_beat = 0;
    return;
  }

  if (intervals[interval_index] > 0) {
    valid_intervals--;
    interval_sum_ms -= intervals[interval_index];
  }

  intervals[interval_index] = time_since_last_beat;
  interval_sum_ms += time_since_last_beat;
  valid_intervals++;
  time_since_last_beat = 0;

  interval_index = (interval_index + 1) % BEATS_FOR_BPM;

  if (valid_intervals > 5) {
    bpm = (valid_intervals * 60) / ((float)interval_sum_ms / 1000);
  } else {
    bpm = 0;
  }
}

void detect_heartbeat(int sample) {
  static beat_state_t state = WAITING_FOR_BEAT;
  static int upper_threshold = 1000;
  static int lower_threshold = 800;
  static int peak = 0;

  time_since_last_beat += SAMPLE_PERIOD_MS;

  switch (state) {
  case WAITING_FOR_BEAT:
    if (sample >= upper_threshold) {
      state = IN_BEAT;
      peak = sample;
    }
    upper_threshold = (upper_threshold > THRESHOLD_FLOOR)
                          ? upper_threshold * THRESHOLD_DECAY
                          : THRESHOLD_FLOOR;

    lower_threshold = (lower_threshold > THRESHOLD_FLOOR)
                          ? lower_threshold * THRESHOLD_DECAY
                          : THRESHOLD_FLOOR;

    break;

  case IN_BEAT:
    if (sample > peak) {
      peak = sample;
    }
    if (sample <= lower_threshold) {
      register_beat();
      upper_threshold = peak * UPPER_RATIO;
      lower_threshold = peak * LOWER_RATIO;
      state = WAITING_FOR_BEAT;
    }
    break;
  }

  if (time_since_last_beat > BEAT_TIMEOUT_MS) {
    memset(intervals, 0, sizeof(intervals));
    valid_intervals = 0;
    interval_sum_ms = 0;
    bpm = 0;
    time_since_last_beat = 0;
    state = WAITING_FOR_BEAT;
  }
}

static void periodic_timer_callback(void *args) {
  int raw_val;
  static float sum = 0;
  static float samples[300] = {0};
  static int count = 0;

  adc_oneshot_read(adc_handle, ADC_CHANNEL_8, &raw_val);

  float raw_float = (float)raw_val;
  sum -= samples[count];
  samples[count] = raw_float;
  sum += samples[count];
  count++;

  if (count >= 300)
    count = 0;
  float offset = sum / 300;
  float norm_val = raw_float - offset;

  norm_val = lowpass_filter(norm_val);
  norm_val = highpass_filter(norm_val);

  int ret = (int)norm_val;

  static int c = 0;
  c++;
  if (c == 20) {
    c = 0;
    detect_heartbeat(ret);
    xQueueSendFromISR(sample_queue, &ret, NULL);
  }
}

void sampler_init(void) {
  sample_queue = xQueueCreate(20, sizeof(int));

  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_2,
  };

  adc_oneshot_new_unit(&init_config, &adc_handle);

  adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_12,
  };

  adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_8, &config);

  const esp_timer_create_args_t timer_args = {
      .callback = &periodic_timer_callback,
      .name = "periodic_timer",
  };
  esp_timer_handle_t timer_handle;
  esp_timer_create(&timer_args, &timer_handle);
  esp_timer_start_periodic(timer_handle, 1000000 / 1000);
}
float highpass_filter(float value) {
  static float ybuf[4] = {0};
  static float xbuf[4] = {0};
  float tot = 0;

  for (int i = highpass_NL - 1; i > 0; i--) {
    xbuf[i] = xbuf[i - 1];
    ybuf[i] = ybuf[i - 1];
  }
  xbuf[0] = value;
  for (int i = 0; i < highpass_NL; i++) {
    tot += xbuf[i] * highpass_NUM[i];
  }
  for (int i = 1; i < highpass_NL; i++) {
    tot -= ybuf[i] * highpass_DEN[i];
  }

  ybuf[0] = tot;
  // printf("hp input: %f, hp output: %f \n",value,tot);
  return -tot;
}
float lowpass_filter(float value) {
  static float l_ybuf[4] = {0};
  static float l_xbuf[4] = {0};
  float tot = 0;

  for (int i = lowpass_NL - 1; i > 0; i--) {
    l_xbuf[i] = l_xbuf[i - 1];
    l_ybuf[i] = l_ybuf[i - 1];
  }

  l_xbuf[0] = value;
  for (int i = 0; i < lowpass_NL; i++) {
    tot += l_xbuf[i] * lowpass_NUM[i];
  }

  for (int i = 1; i < lowpass_NL; i++) {
    tot -= l_ybuf[i] * lowpass_DEN[i];
  }

  l_ybuf[0] = tot;
  return tot;
}
