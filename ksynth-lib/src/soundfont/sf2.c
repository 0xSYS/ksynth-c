#include <stdio.h>
#include <stdlib.h>
#include <log_c/log.h>


#include "sf2.h"
#include "../ksynth.h"



/*
Todo: Get sample loop
*/



// MARK: Internal funcs
// Read PHDR just for info (optional)
void read_phdr(FILE* f, uint32_t size)
{
  if(size % sizeof(PresetHeader) != 0)
  {
    log_error("ERROR: phdr chunk size (%u) is not a multiple of PresetHeader size (%zu)\n", size, sizeof(PresetHeader));
    return;
  }

  size_t count = size / sizeof(PresetHeader);
  PresetHeader* presets = malloc(size);

  fread(presets, sizeof(PresetHeader), count, f);

  printf("Presets:\n");
  for(size_t i = 0; i < count - 1; i++)\
  {
    // last is terminator
    char name[21];
    memcpy(name, presets[i].presetName, 20);
    name[20] = '\0';
    printf("\033[38;5;30mPreset\033[0m \033[38;5;41m%zu\033[0m: | \033[38;5;51mPreset#\033[0m: \033[38;5;41m%u\033[0m | \033[38;5;118mBank\033[0m: \033[38;5;41m%u\033[0m | \033[38;5;63mBag Index\033[0m: \033[38;5;41m%u\033[0m | \033[38;5;27mPreset Name\033[0m: \033[38;5;112m%s\033[0m\n", i, presets[i].preset, presets[i].bank, presets[i].presetBagIndex, name);
  }

  free(presets);
}

// Read SHDR to get SampleHeader info
SampleHeader* read_shdr(FILE* f, uint32_t size, size_t* out_count)
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
    printf("\033[38;5;33mSample\033[0m \033[38;5;41m%2zu\033[0m: | \033[38;5;208mStart\033[0m: \033[38;5;41m%u\033[0m | \033[38;5;208mEnd\033[0m: \033[38;5;41m%u\033[0m | \033[38;5;200mLS\033[0m: \033[38;5;41m%u\033[0m | \033[38;5;200mLE\033[0m: \033[38;5;41m%u\033[0m | \033[30;5;197mSample Rate\033[0m: \033[38;5;41m%u\033[0m | \033[38;5;226mSample Name\033[0m: \033[38;5;112m%s\033[0m\n", i, samples[i].start, samples[i].end, samples[i].startLoop, samples[i].endLoop, samples[i].sampleRate, name);
  }

  *out_count = count - 1; // exclude EOS terminator
  return samples;
}

// Find a LIST chunk by type (e.g., "pdta", "sdta")
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
        return ftell(f);  // position after list type field
      }
    }
    
    uint32_t padded_size = (chunk.size + 1) & ~1;
    fseek(f, chunk_data_start + padded_size, SEEK_SET);
  }

  return -1;
}

// Find a chunk by ID within a range (offset, size)
long find_chunk_in(FILE* f, const char* chunk_id, long offset, uint32_t size)
{
  fseek(f, offset, SEEK_SET);

  long end = offset + size;
  ChunkHeader chunk;

  while(ftell(f) < end && fread(&chunk, sizeof(chunk), 1, f) == 1)
  {
      if(memcmp(chunk.id, chunk_id, 4) == 0)
          return ftell(f);
  
      uint32_t padded_size = (chunk.size + 1) & ~1;
      fseek(f, padded_size, SEEK_CUR);
  }

  return -1;
}


// Load the smpl chunk data into memory buffer (raw PCM)
int16_t* load_sample_data(FILE* f, long smpl_offset, uint32_t smpl_size)
{
  fseek(f, smpl_offset, SEEK_SET);
  int16_t* data = malloc(smpl_size);
  if(!data)
    return NULL;

  size_t read_count = fread(data, 1, smpl_size, f);
  if(read_count != smpl_size)
  {
    free(data);
    return NULL;
  }
  return data;
}


// MARK: Main loader function
// Main loader function with keyrange extraction
void ksynth_load_sf2_samples(const char* path, struct Sample** out_samples, size_t* out_sample_count)
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

    // Variables for chunks
  SampleHeader* sample_headers = NULL;
  size_t sample_header_count = 0;

  Instrument* instruments = NULL;
  size_t instrument_count = 0;

  InstBag* ibags = NULL;
  size_t ibag_count = 0;

  InstGen* igens = NULL;
  size_t igen_count = 0;

  // Read pdta subchunks
  while((uint32_t)ftell(f) < pdta_end)
  {
    ChunkHeader subchunk;
    if(fread(&subchunk, sizeof(subchunk), 1, f) != 1)
      break;
  
    long sub_start = ftell(f);
  
    if(memcmp(subchunk.id, "phdr", 4) == 0)
    {
      // We can skip, or optionally parse instruments if needed
      read_phdr(f, subchunk.size);
    }
    else if(memcmp(subchunk.id, "shdr", 4) == 0)
    {
      sample_headers = read_shdr(f, subchunk.size, &sample_header_count);
    }
    else if(memcmp(subchunk.id, "inst", 4) == 0)
    {
      instrument_count = subchunk.size / sizeof(Instrument);
      instruments = malloc(subchunk.size);
      fread(instruments, 1, subchunk.size, f);
    }
    else if(memcmp(subchunk.id, "ibag", 4) == 0)
    {
      ibag_count = subchunk.size / sizeof(InstBag);
      ibags = malloc(subchunk.size);
      fread(ibags, 1, subchunk.size, f);
    }
    else if(memcmp(subchunk.id, "igen", 4) == 0)
    {
      igen_count = subchunk.size / sizeof(InstGen);
      igens = malloc(subchunk.size);
      fread(igens, 1, subchunk.size, f);
    }
    else
    {
      fseek(f, sub_start + ((subchunk.size + 1) & ~1), SEEK_SET);
    }
  }

  // Now find sdta LIST chunk for sample data
  long sdta_offset = find_list_chunk(f, "sdta");
  if(sdta_offset < 0)
  {
    log_error("Missing SDTA header !!!");
    fclose(f);
    free(sample_headers);
    free(instruments);
    free(ibags);
    free(igens);
    return;
  }

  fseek(f, sdta_offset - sizeof(ChunkHeader), SEEK_SET);
  ChunkHeader sdta_header;
  if(fread(&sdta_header, sizeof(sdta_header), 1, f) != 1)
  {
    log_error("Failed to read SDTA list chunk !!!");
    fclose(f);
    free(sample_headers);
    free(instruments);
    free(ibags);
    free(igens);
    return;
  }

  uint32_t sdta_end = ftell(f) + sdta_header.size;

  // Find "smpl" chunk inside sdta
  long smpl_offset = -1;
  uint32_t smpl_size = 0;

  while((uint32_t)ftell(f) < sdta_end)
  {
    ChunkHeader chunk;
    if(fread(&chunk, sizeof(chunk), 1, f) != 1)
      break;
  
    long data_start = ftell(f);
  
    if(memcmp(chunk.id, "smpl", 4) == 0)
    {
      smpl_offset = ftell(f);
      smpl_size = chunk.size;
      break;
    }
    fseek(f, data_start + ((chunk.size + 1) & ~1), SEEK_SET);
  }

    if(smpl_offset < 0)
    {
      log_error("Missing smpl chunk inside sdta");
      fclose(f);
      free(sample_headers);
      free(instruments);
      free(ibags);
      free(igens);
      return;
    }

  // Load raw PCM data
  int16_t* sample_data = load_sample_data(f, smpl_offset, smpl_size);
  if(!sample_data)
  {
    log_error("Failed to load sample data");
    fclose(f);
    free(sample_headers);
    free(instruments);
    free(ibags);
    free(igens);
    return;
  }

  // Prepare Samples array
  struct Sample* samples = calloc(sample_header_count, sizeof(struct Sample));
  if(!samples)
  {
    log_error("Out of memory for samples array");
    free(sample_data);
    fclose(f);
    free(sample_headers);
    free(instruments);
    free(ibags);
    free(igens);
    return;
  }

  // Initialize samples audio data pointers, rates and lengths from headers
  for(size_t i = 0; i < sample_header_count; i++)
  {
    samples[i].sample_rate = sample_headers[i].sampleRate;
    samples[i].length = sample_headers[i].end - sample_headers[i].start;
  
    // audio_data points inside sample_data buffer, offset by sample start
    samples[i].audio_data = sample_data + sample_headers[i].start;
  
    // Default keyrange to full (0-127), will update later if found
    samples[i].low_key = 0;
    samples[i].hi_key = 127;

    samples[i].loop_start = sample_headers[i].startLoop - sample_headers[i].start;
    samples[i].loop_end   = sample_headers[i].endLoop   - sample_headers[i].start;
  }

    // Parse instruments to find key range for each sample

  if(instruments && ibags && igens)
  {
    for(size_t inst_i = 0; inst_i < instrument_count - 1; inst_i++) // last is terminator
    {
      uint16_t bag_start = instruments[inst_i].instBagNdx;
      uint16_t bag_end = instruments[inst_i + 1].instBagNdx;
        
      for(uint16_t bag_i = bag_start; bag_i < bag_end; bag_i++)
      {
        uint16_t gen_start = ibags[bag_i].genIndex;
        uint16_t gen_end = (bag_i + 1 < ibag_count) ? ibags[bag_i + 1].genIndex : igen_count;
      
        uint16_t sample_id = 0xFFFF;
        uint8_t key_low = 0;
        uint8_t key_high = 127;
        int has_keyrange = 0;
      
        for(uint16_t gen_i = gen_start; gen_i < gen_end; gen_i++)
        {
          uint16_t gen_oper = igens[gen_i].genOper;
          int16_t gen_amt = igens[gen_i].genAmount;
          
          if(gen_oper == 53) // sampleID generator, index = 53 decimal
          {
            sample_id = (uint16_t)gen_amt;
          }
          else if(gen_oper == GEN_KEYRANGE)
          {
            // genAmount is a WORD with low byte = low key, high byte = high key
            uint8_t low = (uint8_t)(gen_amt & 0xFF);
            uint8_t high = (uint8_t)((gen_amt >> 8) & 0xFF);
            key_low = low;
            key_high = high;
            has_keyrange = 1;
          }
        }
        
        // If sample_id is valid and this is the first time we assign keyrange to that sample
        if(sample_id != 0xFFFF && sample_id < sample_header_count)
        {
          if(!has_keyrange)
          {
            // If no keyrange specified, default full range
            key_low = 0;
            key_high = 127;
          }
        
          // Assign key range only if first assignment (key_low == 0 && key_high == 127 means default)
          // So preserve any previously assigned keyrange from an earlier zone.
          if(samples[sample_id].low_key == 0 && samples[sample_id].hi_key == 127)
          {
            samples[sample_id].low_key = key_low;
            samples[sample_id].hi_key = key_high;
          }
        }
      }
    }
  }

  // Output results
  *out_samples = samples;
  *out_sample_count = sample_header_count;

  // Cleanup
  free(sample_headers);
  free(instruments);
  free(ibags);
  free(igens);

  fclose(f);
}