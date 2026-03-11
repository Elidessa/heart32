#include "sampler.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "hal/adc_types.h"
#include "highpass.h"
#include "lowpass.h"
#include "portmacro.h"
#define BEATS_FOR_BPM 10
void calculate_bpm(int sample);
volatile int bpm;
int bpm_previous = 0;
volatile int time_since_last_beat = 0;
volatile int prev_sample = 0;
char bpm_str[10];

volatile unsigned int beat_times[BEATS_FOR_BPM] = {0};
volatile int beat[BEATS_FOR_BPM] = {0};

static adc_oneshot_unit_handle_t adc_handle;
QueueHandle_t sample_queue = NULL;
float highpass_filter(float value);
float lowpass_filter(float value);

void calculate_bpm(int sample){
		static int is_rising, over_upper_threshold;

		time_since_last_beat += 20;
		if(sample > prev_sample){
				is_rising = 1;
		}else if(is_rising && sample < prev_sample){
				is_rising = 0;
		}

		static int upper_threshold = 1000;
		static int lower_threshold = 800;
		//calc BPM
		printf("upper: %d, lower: %d, sample: %d , bpm: %d \n",upper_threshold,lower_threshold, sample, bpm);

		int t_sum = 0;
		int c = 0;
		for(int i = 0; i < BEATS_FOR_BPM; i++){
				if(beat_times[i] > 0){
						t_sum+=beat_times[i];
						c++;
				}
		}
		if(c > 5){
				bpm = (c*60)/((float)t_sum / 1000);
				printf("calc beat\n");
		}
		//Beat detected, time and beat added to arrays
		if(sample >= upper_threshold && is_rising){
				over_upper_threshold = 1;
				upper_threshold = prev_sample*0.75;
				lower_threshold = prev_sample*0.5;
		}else if(sample <= lower_threshold && !is_rising && over_upper_threshold){
				over_upper_threshold = 0;
				for(int i = BEATS_FOR_BPM - 1; i > 0; i--){
						beat_times[i] = beat_times[i - 1];
				}
				beat_times[0] = time_since_last_beat;
				time_since_last_beat = 0;
		}
		prev_sample = sample;

		//beat not detected for 2 seconds, remove one beat. 
		if(time_since_last_beat > 2000){
				for(int i = BEATS_FOR_BPM - 1; i > 0; i--){
						beat_times[i] = beat_times[i - 1];
				}
				beat_times[0] = 0;
				bpm = 0;
		}
		upper_threshold = (upper_threshold < 200) ? 200 : upper_threshold * 0.99;
		lower_threshold = (lower_threshold < 200) ? 200 : lower_threshold * 0.99;
}
static void periodic_timer_callback(void *args){
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

		if(count >= 300) count = 0;
		float offset = sum/300;
		float norm_val = raw_float-offset;

		norm_val = lowpass_filter(norm_val);
		norm_val = highpass_filter(norm_val);

		int ret = (int)norm_val;

		static int c = 0;
		c++;
		if(c == 20){
				c = 0;
				calculate_bpm(ret);
				xQueueSendFromISR(sample_queue, &ret,NULL);
		}
}


void sampler_init(void){
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
	esp_timer_start_periodic(timer_handle, 1000000/1000);
}
float highpass_filter(float value){
		static float ybuf[4] = {0};
		static float xbuf[4] = {0};
		float tot = 0;

		for (int i = highpass_NL - 1; i > 0; i--){
				xbuf[i] = xbuf[i - 1];
				ybuf[i] = ybuf[i - 1];
		}
		xbuf[0] = value;
		for(int i = 0; i < highpass_NL; i++){
				tot += xbuf[i]*highpass_NUM[i]; 
		}
		for(int i = 1; i < highpass_NL; i++){
				tot -= ybuf[i]*highpass_DEN[i];
		}
	
		ybuf[0] = tot;
		//printf("hp input: %f, hp output: %f \n",value,tot);
		return -tot;
}
float lowpass_filter(float value){
		static float l_ybuf[4] = {0};
		static float l_xbuf[4] = {0};
		float tot = 0;

		for (int i = lowpass_NL - 1; i > 0; i--){
				l_xbuf[i] = l_xbuf[i -1];
				l_ybuf[i] = l_ybuf[i -1];
		}

		l_xbuf[0] = value;
		for(int i = 0; i < lowpass_NL; i++){
				tot += l_xbuf[i]*lowpass_NUM[i];
		}

		for(int i = 1; i < lowpass_NL; i++){
				tot -= l_ybuf[i]*lowpass_DEN[i];
		}

		l_ybuf[0] = tot;
		return tot;
}
