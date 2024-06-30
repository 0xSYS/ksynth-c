#include "voice.h"
#include <stdlib.h>

void voice_free(struct Voice* voice) {
	if(voice != NULL) {
		free(voice);
	}
}

void voice_reset(struct Voice* voice) {
	voice->killed = 1;

	voice->channel = 0;
	voice->curfalloff = 0;
	voice->noteNumber = 0;
	voice->sample_position = 0;

	voice->tokill = 0;
	voice->velocity = 0;
}
