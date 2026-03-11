#ifndef SAMPLER_H
#define SAMPLER_H

#include "freertos/idf_additions.h"

extern QueueHandle_t sample_queue;
extern volatile int bpm;
void sampler_init(void);

#endif
