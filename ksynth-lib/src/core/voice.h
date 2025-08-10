#ifndef VOICE_H
#define VOICE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

struct Voice
{
	unsigned char killed;
	unsigned char tokill;
	unsigned char channel;
	unsigned char noteNumber;
	unsigned char velocity;
	unsigned int curfalloff;
	unsigned int sample_position;
  struct Sample* sample;
};

void voice_free(struct Voice* voice);
void voice_reset(struct Voice* voice);

#ifdef __cplusplus
}
#endif
#endif