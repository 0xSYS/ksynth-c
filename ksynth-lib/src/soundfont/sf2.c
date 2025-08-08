#include <stdio.h>
#include <stdlib.h>
#include <log_c/log.h>


#include "sf2.h"
#include "../ksynth.h"



/*
Todo: Store sample data into Sample.audio_data
*/


void read_phdr(FILE* f, uint32_t size)
{
  size_t count = size / sizeof(PresetHeader);
  PresetHeader* presets = malloc(size);
  fread(presets, 1, size, f);

  printf("Presets:\n");
  for (size_t i = 0; i < count - 1; i++) // last is EOP terminator
  {
    printf("Presets -> Index: [%2zu] Preset Name: %s | Bank %d, Preset %d\n", i, presets[i].presetName, presets[i].bank, presets[i].preset);
  }

  free(presets);
}

void read_shdr(FILE* f, uint32_t size)
{
  size_t count = size / sizeof(SampleHeader);
  SampleHeader* samples = malloc(size);
  fread(samples, 1, size, f);

  

  printf("Samples:\n");
  for(size_t i = 0; i < count - 1; i++) // last is EOS terminator
  {
    char name[21];
    memcpy(name, samples[i].sampleName, 20);
    name[20] = '\0';
    printf("Samples -> Index: [%2zu] | Sample Name: %s | Start: %u, End: %u, Rate: %u\n", i, name, samples[i].start, samples[i].end, samples[i].sampleRate);
  }

  free(samples);

}

long find_list_chunk(FILE* f, const char* target_type)
{
  fseek(f, sizeof(RiffChunkHeader), SEEK_SET);  // Skip RIFF header

  ChunkHeader chunk;
  while(fread(&chunk, sizeof(chunk), 1, f) == 1)
  {
    long chunk_data_start = ftell(f);

    if(memcmp(chunk.id, "LIST", 4) == 0)
    {
      char list_type[4];
      if(fread(list_type, 1, 4, f) != 4)
        break;
          
      if(memcmp(list_type, target_type, 4) == 0)
      {
        return ftell(f);  // Found target
      }
    }
      
    uint32_t padded_size = (chunk.size + 1) & ~1;
    fseek(f, chunk_data_start + padded_size, SEEK_SET);
  }
    
  return -1;
}


void ksynth_load_soundfont_samples(const char * path)
{
  FILE* f = fopen(path, "rb");
  if(!f)
  {
    log_error("Failed to open soundfont file !!!");
    perror("fopen");
    return;
  }

  RiffChunkHeader riff;
  fread(&riff, sizeof(riff), 1, f);

  if(memcmp(riff.id, "RIFF", 4) != 0 || memcmp(riff.type, "sfbk", 4) != 0)
  {
    log_error("Invalid soundfont file");
    fclose(f);
    return;
  }

  long pdta_offset = find_list_chunk(f, "pdta");
  if(pdta_offset < 0)
  {
    log_error("Missing PDTA header !!!");
    fclose(f);
    return;
  }

  fseek(f, pdta_offset - sizeof(ChunkHeader), SEEK_SET);

  ChunkHeader pdta_header;
  if(fread(&pdta_header, sizeof(pdta_header), 1, f) != 1)
  {
    log_error("Failed to read PDTA list chunk !!!");
    fclose(f);
    return;
  }

  uint32_t pdta_end = ftell(f) + pdta_header.size;

  while((uint32_t)ftell(f) < pdta_end)
  {
      ChunkHeader subchunk;
      if(fread(&subchunk, sizeof(subchunk), 1, f) != 1)
          break;
      
      long sub_start = ftell(f);
      
      if(memcmp(subchunk.id, "phdr", 4) == 0)
      {
        read_phdr(f, subchunk.size);
      }
      else if(memcmp(subchunk.id, "shdr", 4) == 0)
      {
        read_shdr(f, subchunk.size);
      }
      else
      {
        fseek(f, sub_start + ((subchunk.size + 1) & ~1), SEEK_SET);
      }
  }
    
  fclose(f);
}
