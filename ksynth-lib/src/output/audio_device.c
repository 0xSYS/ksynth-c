#include <stdint.h>
#include <stdlib.h>

#include <log_c/log.h>


#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>


#include "audio_device.h"

#include "../core/sample.h"



struct Voice g_voices[MAX_VOICES];
ma_device device;



void load_soundfont_sample(struct Sample* s, int16_t* pcm, unsigned int sample_rate, unsigned int length_frames, uint8_t low_key, uint8_t hi_key, unsigned int loop_start, unsigned int loop_end)
{
  s->audio_data  = pcm;
  s->sample_rate = sample_rate;
  s->length      = length_frames;
  s->low_key     = low_key;
  s->hi_key      = hi_key;
  s->loop_start  = loop_start;
  s->loop_end    = loop_end;
}

void sample_play(struct Voice* v, struct Sample* s, unsigned char channel, unsigned char noteNumber, unsigned char velocity)
{
  v->sample          = s;
  v->channel         = channel;
  v->noteNumber      = noteNumber;
  v->velocity        = velocity;
  v->sample_position = 0;
  v->tokill          = 0;
  v->killed          = 0;
}

void sample_stop(struct Voice* v)
{
  v->tokill = 1;
}

void audio_cb(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
  int16_t* out = (int16_t*)pOutput;
  memset(out, 0, frameCount * pDevice->playback.channels * sizeof(int16_t));

  for(int v = 0; v < MAX_VOICES; v++)
  {
    struct Voice* voice = &g_voices[v];
    if(!voice->sample || voice->killed)
      continue;
    
    struct Sample* s = voice->sample;
    for(ma_uint32 f = 0; f < frameCount; f++)
    {
      if(voice->sample_position >= s->length)
      {
        voice->killed = 1;
        break;
      }
      
      int16_t sampleVal = s->audio_data[voice->sample_position];
      float volume = voice->velocity / 127.0f;
      int mixed = (int)(sampleVal * volume);
      
      // Stereo: same value in both channels
      int idx = f * pDevice->playback.channels;
      int left  = out[idx]     + mixed;
      int right = out[idx + 1] + mixed;
      
      // Clip to int16 range
      out[idx]     = (left  > 32767 ? 32767 : (left  < -32768 ? -32768 : left));
      out[idx + 1] = (right > 32767 ? 32767 : (right < -32768 ? -32768 : right));
      
      voice->sample_position++;
    }
  }
  (void)pInput;
}

bool init_audio_eng(struct Voice vc[])
{
  // Initialize voices to empty
  vc = g_voices;
  memset(g_voices, 0, sizeof(g_voices));

  // Init device
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format   = ma_format_s16;
  config.playback.channels = 2;
  config.sampleRate        = 48000;   // match your samples if possible
  config.dataCallback      = audio_cb;

  if(ma_device_init(NULL, &config, &device) != MA_SUCCESS)
  {
    log_error("Failed to init device\n");
    return false;
  }

  ma_device_start(&device);

  return true;

    // Example usage:
    //struct Sample s1 = {/* fill with your PCM data */};
    //sample_load(&s1, &g_voices[0]);
    //sample_play(&g_voices[0]);

    //getchar(); // wait
    
}

void destroy_audio_eng()
{
  ma_device_uninit(&device);
}