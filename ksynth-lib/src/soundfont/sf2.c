#include <stdio.h>
#include <stdlib.h>
#include <log_c/log.h>


#include "sf2.h"
#include "../ksynth.h"



/*
Todo: Get sample keyrange
*/



// Reads preset headers
void read_phdr(FILE* f, uint32_t size)
{
  size_t count = size / sizeof(PresetHeader);
  PresetHeader* presets = malloc(size);
  fread(presets, 1, size, f);

  printf("Presets:\n");
  for(size_t i = 0; i < count - 1; i++) // last is EOP terminator
  {
    printf("Presets -> Index: [%2zu] Preset Name: %s | Bank %d, Preset %d\n", i, presets[i].presetName, presets[i].bank, presets[i].preset);
  }

  free(presets);
}

// Finds a LIST chunk by type
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
          return ftell(f);  // position after LIST + type
        }
    }
  
    uint32_t padded_size = (chunk.size + 1) & ~1;
    fseek(f, chunk_data_start + padded_size, SEEK_SET);
  }

  return -1;
}

// Finds a specific chunk inside a given LIST range
long find_chunk_in(FILE* f, const char* target_id, long start_offset, uint32_t list_size)
{
  fseek(f, start_offset, SEEK_SET);
  long list_end = start_offset + list_size;

  while(ftell(f) < list_end)
  {
    ChunkHeader chunk;
    if(fread(&chunk, sizeof(chunk), 1, f) != 1)
      break;

    long data_start = ftell(f);

    if(memcmp(chunk.id, target_id, 4) == 0)
    {
      return data_start;
    }

    uint32_t padded_size = (chunk.size + 1) & ~1;
    fseek(f, data_start + padded_size, SEEK_SET);
  }
  return -1;
}

// Reads all sample headers and fills sf2_samples
void read_shdr_and_samples(FILE* f, uint32_t size, struct Sample** sf2_samples, int16_t* smpl_data, uint32_t smpl_samples_count)
{
  size_t count = size / sizeof(SampleHeader);
  SampleHeader* samples = malloc(size);
  fread(samples, 1, size, f);

  *sf2_samples = malloc((count - 1) * sizeof(struct Sample));

  printf("Samples:\n");
  for(size_t i = 0; i < count - 1; i++) // last is EOS terminator
  {
    char name[21];
    memcpy(name, samples[i].sampleName, 20);
    name[20] = '\0';

    uint32_t start = samples[i].start;
    uint32_t end   = samples[i].end;

    if(end > smpl_samples_count)
      end = smpl_samples_count;

    uint32_t length = end - start;

    (*sf2_samples)[i].audio_data  = malloc(length * sizeof(int16_t));
    memcpy((*sf2_samples)[i].audio_data, &smpl_data[start], length * sizeof(int16_t));
    (*sf2_samples)[i].sample_rate = samples[i].sampleRate;
    (*sf2_samples)[i].length      = length;

    printf("Samples -> Index: [%2zu] | Name: %s | Start: %u, End: %u, Rate: %u, Len: %u\n", i, name, start, end, samples[i].sampleRate, length);
  }
    
  free(samples);
}

void ksynth_load_sf2_samples(const char * path, struct Sample** sf2_samples, size_t* sample_count)
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

  // Locate sdta
  long sdta_offset = find_list_chunk(f, "sdta");
  if(sdta_offset < 0)
  {
    log_error("Missing sdta LIST chunk !!!");
    fclose(f);
    return;
  }

  // Read sdta LIST header to get size
  fseek(f, sdta_offset - sizeof(ChunkHeader) - 4, SEEK_SET);
  ChunkHeader sdta_list;
  fread(&sdta_list, sizeof(sdta_list), 1, f);
  fseek(f, 4, SEEK_CUR); // skip "sdta" type

  // Locate smpl chunk inside sdta
  long smpl_offset = find_chunk_in(f, "smpl", sdta_offset, sdta_list.size - 4);
  if(smpl_offset < 0)
  {
    log_error("Missing smpl chunk inside sdta");
    fclose(f);
    return;
  }

  // Read smpl chunk size
  fseek(f, smpl_offset - sizeof(ChunkHeader), SEEK_SET);
  ChunkHeader smpl_chunk;
  fread(&smpl_chunk, sizeof(smpl_chunk), 1, f);

  uint32_t smpl_samples_count = smpl_chunk.size / sizeof(int16_t);
  int16_t* smpl_data = malloc(smpl_chunk.size);
  fread(smpl_data, 1, smpl_chunk.size, f);

  // Locate pdta
  long pdta_offset = find_list_chunk(f, "pdta");
  if(pdta_offset < 0)
  {
    log_error("Missing PDTA header !!!");
    free(smpl_data);
    fclose(f);
    return;
  }

  fseek(f, pdta_offset - sizeof(ChunkHeader), SEEK_SET);
  ChunkHeader pdta_header;
  fread(&pdta_header, sizeof(pdta_header), 1, f);
  uint32_t pdta_end = ftell(f) + pdta_header.size;

  // Parse PDTA
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
      read_shdr_and_samples(f, subchunk.size, sf2_samples, smpl_data, smpl_samples_count);
      *sample_count = (subchunk.size / sizeof(SampleHeader)) - 1;
    }
    else
    {
      fseek(f, sub_start + ((subchunk.size + 1) & ~1), SEEK_SET);
    }
  }

  free(smpl_data);
  fclose(f);
}