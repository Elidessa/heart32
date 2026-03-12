#ifndef FILTER_H
#define FILTER_H

#define MAX_FILTER_ORDER 3

typedef struct {
    const float *b;                        /* numerator coefficients */
    const float *a;                        /* denominator coefficients */
    int order;                             /* filter order */
    float xbuf[MAX_FILTER_ORDER + 1];      /* input history buffer */
    float ybuf[MAX_FILTER_ORDER + 1];      /* output history buffer */
} iir_filter_t;

static inline float iir_filter_step(iir_filter_t *filter, float input) {
    for (int i = filter->order; i > 0; i--) {
        filter->xbuf[i] = filter->xbuf[i - 1];
    }
    filter->xbuf[0] = input;

    float output = 0.0f;
    for (int i = 0; i <= filter->order; i++) {
        output += filter->b[i] * filter->xbuf[i];
    }

    for (int i = 1; i <= filter->order; i++) {
        output -= filter->a[i] * filter->ybuf[i - 1];
    }

    for (int i = filter->order; i > 0; i--) {
        filter->ybuf[i] = filter->ybuf[i - 1];
    }

    filter->ybuf[0] = output;
	
    return output;
}

#endif
