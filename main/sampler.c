#include "sampler.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "hal/adc_types.h"

static adc_oneshot_unit_handle_t adc_handle;
QueueHandle_t sample_queue = NULL;


static void periodic_timer_callback(void *args){
	int raw_val;
	static int samples[300] = {0};
		static int count = 0;
	adc_oneshot_read(adc_handle, ADC_CHANNEL_8, &raw_val);
	samples[count] = raw_val;
		count++;
	if(count > 300) count = 0;
	int sum = 0;
	for(int i = 0; i < 300; i++){
				sum += samples[i];

	}
	sum /= 300;
	raw_val -= sum;
		printf("%d\n",raw_val);
	xQueueSendFromISR(sample_queue, &raw_val, NULL);
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
	esp_timer_start_periodic(timer_handle, 100000);
}
