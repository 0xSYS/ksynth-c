#ifndef SF2_H
#define SF2_H

#include <stdint.h>
#include <stdbool.h>

#include "../utils.h"





//void ksynth_load_soundfont(const char * path);

KSYNTH_API bool load_preset_to_sample(const char* sf2_path, int bank, int preset, int midi_note, float duration_seconds, struct Sample* out_sample);

#endif