#ifndef AUDIO_DEVICE_H
#define AUDIO_DEVIFE_H

#include <stdbool.h>
#include "../core/sample.h"
#include "../core/voice.h"


#define MAX_VOICES 1000



bool init_audio_eng();
void load_soundfont_sample(struct Sample* s, int16_t* pcm, unsigned int sample_rate, unsigned int length_frames, uint8_t low_key, uint8_t hi_key, unsigned int loop_start, unsigned int loop_end);
void sample_play(struct Voice* v, struct Sample* s, unsigned char channel, unsigned char noteNumber, unsigned char velocity);
void sample_stop(struct Voice* v);
void destroy_audio_eng();

#endif