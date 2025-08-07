#ifndef SAMPLE_H
#define SAMPLE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct Sample
{
	int16_t* audio_data;
	unsigned int sample_rate;
	unsigned int length;
};

void sample_free(struct Sample* sample);

#ifdef __cplusplus
}
#endif
#endif