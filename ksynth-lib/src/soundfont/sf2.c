#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#include "sf2.h"
#include "../core/sample.h"

#define TSF_IMPLEMENTATION
#include <tsf.h>


















bool load_preset_to_sample(const char* sf2_path, int bank, int preset, int midi_note, float duration_seconds, struct Sample* out_sample)
{
    tsf* synth = tsf_load_filename(sf2_path);
    if (!synth)
    {
      fprintf(stderr, "Failed to load SF2: %s\n", sf2_path);
      return false;
    }

    int samplerate = 44100;
    tsf_set_output(synth, TSF_STEREO_INTERLEAVED, samplerate, 0.0f);

    // How many frames to render?
    int frame_count = (int)(duration_seconds * samplerate);

    // Each frame has 2 samples (stereo)
    int total_samples = frame_count * 2;

    // Allocate audio buffer
    int16_t* buffer = malloc(sizeof(int16_t) * total_samples);
    if(!buffer)
    {
      fprintf(stderr, "Out of memory\n");
      tsf_close(synth);
      return false;
    }

    // Note ON
    tsf_note_on(synth, preset, midi_note, 1.0f);

    // Render audio
    tsf_render_short(synth, buffer, frame_count, 0);

    // Note OFF
    tsf_note_off(synth, preset, midi_note);

    // Store in Sample struct
    out_sample->audio_data = buffer;
    out_sample->sample_rate = samplerate;
    out_sample->length = frame_count;

    tsf_close(synth);
    return true;
}
