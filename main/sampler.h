#ifndef SAMPLER_H
#define SAMPLER_H

#include "freertos/idf_additions.h"

extern QueueSetHandle_t sample_queue;
void sampler_init(void);

#endif
