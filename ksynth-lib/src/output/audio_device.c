#include <stdint.h>
#include <stdlib.h>


#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>


#include "audio_device.h"

#include "../core/sample.h"

ma_device device;


void audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
  int16_t* out = (int16_t*)pOutput;
  memset(out, 0, frameCount * sizeof(int16_t) * 2); // stereo zeroed

  // Example: Mix one sample (mono)
  //if(your_sample_is_active)
  //{
  //  for(ma_uint32 i = 0; i < frameCount; ++i)
  //  {
  //    out[i * 2 + 0] += your_sample_data[your_sample_pos]; // Left
  //    out[i * 2 + 1] += your_sample_data[your_sample_pos]; // Right
  //    your_sample_pos++;
  //    
  //    if(your_sample_pos >= your_sample_length)
  //    {
  //      your_sample_is_active = false;
  //      break;
  //    }
  //  }
  //}
}

bool ksynth_init_audio_dev()
{
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.sampleRate = 44100;
  config.playback.format = ma_format_s16;
  config.playback.channels = 2;
  config.dataCallback = audio_callback;
  
  if(ma_device_init(NULL, &config, &device) != MA_SUCCESS)
  {
    fprintf(stderr, "Failed to init device\n");
    return false;
  }
  
  if(ma_device_start(&device) != MA_SUCCESS)
  {
    fprintf(stderr, "Failed to start device\n");
    ma_device_uninit(&device);
    return false;
  }
  return true;
}
