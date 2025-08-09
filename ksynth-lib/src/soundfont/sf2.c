#include <stdio.h>
#include <stdlib.h>
#include <log_c/log.h>


#include "sf2.h"
#include "../ksynth.h"



/*
Todo: Get sample effects
*/



// MARK: Internal funcs
/*
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
*/

static void skip_padded(FILE* f, uint32_t size)
{
    uint32_t padded = (size + 1) & ~1u; // word alignment
    fseek(f, padded, SEEK_CUR);
}

/* find the position right after the LIST type bytes for the first LIST whose type matches 'type_str' */
static long find_list_start(FILE* f, const char* type_str)
{
    if (!f || !type_str) return -1;
    fseek(f, 0, SEEK_SET);

    // skip RIFF header first 12 bytes (RIFF id + size + type)
    if (fseek(f, 12, SEEK_SET) != 0) return -1;

    while (1) {
        ChunkHeader ch;
        if (fread(&ch, sizeof(ch), 1, f) != 1) break;
        long hdr_pos = ftell(f) - sizeof(ch);
        if (memcmp(ch.id, "LIST", 4) == 0) {
            char t[4];
            if (fread(t, 1, 4, f) != 4) break;
            if (memcmp(t, type_str, 4) == 0) {
                // return position just after the 4 type bytes (start of list payload)
                return ftell(f);
            } else {
                // skip remaining list payload (size includes the 4 bytes of type)
                long skip = (long)ch.size - 4;
                long padded = (skip + 1) & ~1L;
                if (fseek(f, hdr_pos + sizeof(ChunkHeader) + 4 + padded, SEEK_SET) != 0) break;
            }
        } else {
            // skip this chunk's data (padded)
            long padded = (long)((ch.size + 1) & ~1u);
            if (fseek(f, hdr_pos + sizeof(ChunkHeader) + padded, SEEK_SET) != 0) break;
        }
    }
    return -1;
}

/* find a chunk with id (eg "smpl") inside a LIST region given by list_data_start and list_data_size.
   returns data_start (position right after the ChunkHeader) or -1 if not found. */
static long find_chunk_in_region(FILE* f, const char* id, long region_start, uint32_t region_size)
{
    if (!f) return -1;
    long end = region_start + region_size;
    long pos = region_start;
    fseek(f, pos, SEEK_SET);

    while (ftell(f) + (long)sizeof(ChunkHeader) <= end) {
        ChunkHeader ch;
        if (fread(&ch, sizeof(ch), 1, f) != 1) break;
        long data_start = ftell(f);
        if (memcmp(ch.id, id, 4) == 0) {
            return data_start;
        }
        // skip padded data
        uint32_t padded = (ch.size + 1) & ~1u;
        pos = data_start + padded;
        if (fseek(f, pos, SEEK_SET) != 0) break;
    }
    return -1;
}

/* Load raw smpl data at smpl_data_pos (data_start) of size bytes */
static int16_t* load_smpl_buffer(FILE* f, long smpl_data_pos, uint32_t smpl_size)
{
    if (!f || smpl_data_pos < 0 || smpl_size == 0) return NULL;
    if (fseek(f, smpl_data_pos, SEEK_SET) != 0) return NULL;
    int16_t* buf = (int16_t*)malloc(smpl_size);
    if (!buf) return NULL;
    size_t r = fread(buf, 1, smpl_size, f);
    if (r != smpl_size) { free(buf); return NULL; }
    return buf;
}


// MARK: Main loader function
// Main loader function with keyrange extraction
//ksynth_sf2_load
/*
int ksynth_sf2_load(const char *path, SampleHeader **samples_out, size_t *sample_count_out, PresetEffects **effects_out, size_t *preset_count_out)
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
  */


int ksynth_sf2_load(const char* path, struct Sample** out_samples, unsigned int* out_sample_count, PresetEffects** out_presets, unsigned int* out_preset_count, int16_t*** out_preset_generators /* optional, can be NULL */)
{
  if(!path || !out_samples || !out_sample_count)
    return -1;

  *out_samples = NULL;
  *out_sample_count = 0;


  if(out_presets)
    *out_presets = NULL;

  if(out_preset_count)
    *out_preset_count = 0;

  if(out_preset_generators)
    *out_preset_generators = NULL;

  FILE* f = fopen(path, "rb");
  if(!f)
  {
    perror("fopen");
    return -2;
  }

  // Read RIFF header
  RiffChunkHeader riff;
  if(fread(&riff, sizeof(riff), 1, f) != 1)
  {
    fclose(f); return -3; 
  }

  if(memcmp(riff.id, "RIFF", 4) != 0 || memcmp(riff.type, "sfbk", 4) != 0)
  {
    fclose(f);
    return -4;
  }

  // Find LIST pdta and sdta starts
  long pdta_list_data = find_list_start(f, "pdta");
  long sdta_list_data = find_list_start(f, "sdta");
  if(pdta_list_data < 0 || sdta_list_data < 0)
  {
    fclose(f);
    return -5;
  }

  /* --- parse PDTA subchunks --- */
  // We must find the LIST header for pdta to get its size. find_list_start returned data position (after type).
  // Seek back a bit to read the LIST header. We'll search for the LIST with type 'pdta' again and read its ChunkHeader.
  fseek(f, 12, SEEK_SET); // skip RIFF header
  long pdta_chunk_header_pos = -1;
  ChunkHeader ch;
  while(fread(&ch, sizeof(ch), 1, f) == 1)
  {
    if(memcmp(ch.id, "LIST", 4) == 0)
    {
      char t[4];
      if (fread(t, 1, 4, f) != 4) break;
  
      if(memcmp(t, "pdta", 4) == 0)
      {
        // Save position pointing to LIST chunk start
        pdta_chunk_header_pos = ftell(f) - sizeof(t) - sizeof(ch);
        break;
      }
  
      // Skip the rest of LIST
      uint32_t skip = (ch.size - 4 + 1) & ~1u;
      fseek(f, skip, SEEK_CUR);
    }
    else
    {
      // Skip non-LIST chunks
      uint32_t skip = (ch.size + 1) & ~1u;
      fseek(f, skip, SEEK_CUR);
    }
  }
  if(pdta_chunk_header_pos < 0)
  {
    fclose(f);
    return -6;
  }

  // read pdta header chunk to get size
  fseek(f, pdta_chunk_header_pos, SEEK_SET);
  if(fread(&ch, sizeof(ch), 1, f) != 1)
  { 
    fclose(f);
    return -7;
  }

  // next 4 bytes are 'pdta', skip them
  fseek(f, 4, SEEK_CUR);
  long pdta_data_start = ftell(f);
  long pdta_data_end = pdta_data_start + (ch.size - 4);

  // Containers for pdta data
  PresetHeader* phdr = NULL; size_t phdr_count = 0;
  InstBag* pbag = NULL; size_t pbag_count = 0;
  InstGen* pgen = NULL; size_t pgen_count = 0;
  Instrument* inst = NULL; size_t inst_count = 0;
  InstBag* ibag = NULL; size_t ibag_count = 0;
  InstGen* igen = NULL; size_t igen_count = 0;
  SampleHeader* shdr = NULL; size_t shdr_count = 0;

  // Iterate PDTA subchunks
  while(ftell(f) + (long)sizeof(ChunkHeader) <= pdta_data_end)
  {
    ChunkHeader sub;
    if(fread(&sub, sizeof(sub), 1, f) != 1)
      break;
      
    long data_start = ftell(f);
      
      if(memcmp(sub.id, "phdr", 4) == 0)
      {
        // read preset headers
        phdr_count = sub.size / sizeof(PresetHeader);
        phdr = (PresetHeader*)malloc(sub.size);
        if(!phdr)
        {
          fclose(f);
          return -8; 
        }
         
        if(fread(phdr, sizeof(PresetHeader), phdr_count, f) != phdr_count)
        {
          fclose(f);
          return -9;
        }
      }
      else if(memcmp(sub.id, "pbag", 4) == 0)
      {
        pbag_count = sub.size / sizeof(InstBag);
        pbag = (InstBag*)malloc(sub.size);
        if(!pbag)
        {
          fclose(f);
          return -10;
        }
        if(fread(pbag, sizeof(InstBag), pbag_count, f) != pbag_count)
        { 
          fclose(f); 
          return -11; 
        }
      }
      else if(memcmp(sub.id, "pgen", 4) == 0)
      {
        pgen_count = sub.size / sizeof(InstGen);
        pgen = (InstGen*)malloc(sub.size);
        if(!pgen)
        {
          fclose(f);
          return -12; 
        }
         
        if(fread(pgen, sizeof(InstGen), pgen_count, f) != pgen_count)
        {
          fclose(f);
          return -13; 
        }
      }
      else if(memcmp(sub.id, "inst", 4) == 0)
      {
        inst_count = sub.size / sizeof(Instrument);
        inst = (Instrument*)malloc(sub.size);
        if(!inst)
        {
          fclose(f);
          return -14;
        }
        if(fread(inst, sizeof(Instrument), inst_count, f) != inst_count)
        {
          fclose(f);
          return -15;
        }
       
      }
      else if(memcmp(sub.id, "ibag", 4) == 0)
      {
        ibag_count = sub.size / sizeof(InstBag);
        ibag = (InstBag*)malloc(sub.size);
        if(!ibag)
        {
          fclose(f);
          return -16; 
        }
        
        if(fread(ibag, sizeof(InstBag), ibag_count, f) != ibag_count)
        { 
          fclose(f); 
          return -17; 
        }
      }
      else if(memcmp(sub.id, "igen", 4) == 0)
      {
        igen_count = sub.size / sizeof(InstGen);
        igen = (InstGen*)malloc(sub.size);
        if(!igen)
        { 
          fclose(f); 
          return -18; 
        }
        
        if(fread(igen, sizeof(InstGen), igen_count, f) != igen_count)
        {
          fclose(f);
          return -19;
        }
       
      }
      else if(memcmp(sub.id, "shdr", 4) == 0)
      {
        shdr_count = sub.size / sizeof(SampleHeader);
        shdr = (SampleHeader*)malloc(sub.size);
        if(!shdr)
        {
          fclose(f); 
          return -20; 
        }
        
        if(fread(shdr, sizeof(SampleHeader), shdr_count, f) != shdr_count)
        { 
          fclose(f); 
          return -21; 
        }
      }
      else 
      {
        // skip unknown
        uint32_t padded = (sub.size + 1) & ~1u;
        fseek(f, data_start + padded, SEEK_SET);
        continue;
      }
      // file pointer is already at end of data for subchunk (we did direct fread)
  }

  /* --- parse SDTA to get smpl chunk --- */
  // find sdta LIST header (we have sdta_list_data earlier but need its ChunkHeader to get size)
  fseek(f, 12, SEEK_SET);
  long sdta_chunk_header_pos = -1;
  while(fread(&ch, sizeof(ch), 1, f) == 1)
  {
    if(memcmp(ch.id, "LIST", 4) == 0)
    {
      long chunk_start = ftell(f) - sizeof(ch);  // start of the LIST chunk header
      char t[4];
      if(fread(t, 1, 4, f) != 4)
        break;
  
      if(memcmp(t, "sdta", 4) == 0)
      {
        sdta_chunk_header_pos = chunk_start;
        break;
      }
     
      // Skip the rest of LIST chunk payload (size excludes the 4 bytes for the list type)
      uint32_t skip = (ch.size - 4 + 1) & ~1u;
      fseek(f, skip, SEEK_CUR);
    }
    else
    {
      // Skip other chunks
      uint32_t skip = (ch.size + 1) & ~1u;
      fseek(f, skip, SEEK_CUR);
    }
  }
  if(sdta_chunk_header_pos < 0)
  {
    fclose(f);
    return -22;
  }

  fseek(f, sdta_chunk_header_pos, SEEK_SET);
  if(fread(&ch, sizeof(ch), 1, f) != 1)
  { 
    fclose(f); 
    return -23; 
  }

  // skip 'sdta'
  fseek(f, 4, SEEK_CUR);
  long sdta_data_start = ftell(f);
  uint32_t sdta_payload_size = ch.size - 4;
  long sdta_data_end = sdta_data_start + sdta_payload_size;

  long smpl_data_pos = -1;
  uint32_t smpl_size = 0;
  while(ftell(f) + (long)sizeof(ChunkHeader) <= sdta_data_end)
  {
    ChunkHeader c2;
    if(fread(&c2, sizeof(c2), 1, f) != 1)
      break;
  
    long data_start = ftell(f);
    if(memcmp(c2.id, "smpl", 4) == 0)
    {
      smpl_data_pos = data_start;
      smpl_size = c2.size;
      break;
    }
    uint32_t padded = (c2.size + 1) & ~1u;
    fseek(f, data_start + padded, SEEK_SET);
  }
  
  if(smpl_data_pos < 0)
  {
    /* cleanup */
    fclose(f);
    return -24;
  }
  
  // load smpl buffer
  int16_t* smpl_buf = load_smpl_buffer(f, smpl_data_pos, smpl_size);
  if(!smpl_buf)
  {
    fclose(f);
    return -25; 
  }
  
  size_t smpl_total_samples = smpl_size / sizeof(int16_t);
  
  /* --- Build Sample[] array and copy each sample's PCM into its own buffer --- */
  if(!shdr || shdr_count < 1)
  {
    free(smpl_buf);
    fclose(f);
    return -26;
  }
  
  size_t sample_count = (shdr_count > 0) ? (shdr_count - 1) : 0; // last is terminator
  struct Sample* samples = (struct Sample*)calloc(sample_count, sizeof(struct Sample));
  if(!samples)
  {
    free(smpl_buf);
    fclose(f);
    return -27; 
  }
  
  for(size_t i = 0; i < sample_count; ++i)
  {
    uint32_t s = shdr[i].start;
    uint32_t e = shdr[i].end;
    if(s >= smpl_total_samples)
    {
      samples[i].audio_data = NULL;
      samples[i].length = 0;
    }
    else
    {
      if(e > smpl_total_samples)
        e = smpl_total_samples;
        
      uint32_t len = (e > s) ? (e - s) : 0;
      samples[i].length = len;
      samples[i].sample_rate = shdr[i].sampleRate;
      // allocate and copy audio data
      if(len > 0)
      {
        samples[i].audio_data = (int16_t*)malloc(len * sizeof(int16_t));
        if(!samples[i].audio_data)
        {
          /* cleanup later */
          samples[i].length = 0;
          samples[i].audio_data = NULL;
        }
      
        else
          memcpy(samples[i].audio_data, smpl_buf + s, len * sizeof(int16_t));
      }
      else
      {
        samples[i].audio_data = NULL;
      }
    }
    // loop points relative to sample start
    samples[i].loop_start = (shdr[i].startLoop > shdr[i].start) ? (shdr[i].startLoop - shdr[i].start) : 0;
    samples[i].loop_end   = (shdr[i].endLoop   > shdr[i].start) ? (shdr[i].endLoop - shdr[i].start) : 0;
    // default keyrange
    samples[i].low_key = 0;
    samples[i].hi_key  = 127;
  }

  /* --- Instrument-level scan to assign keyRange to samples (from igen) --- */
  if(inst && ibag && igen)
  {
    for(size_t ins_i = 0; ins_i + 1 < inst_count; ++ins_i)
    {
      // last is terminator
      uint16_t bag_start = inst[ins_i].instBagNdx;
      uint16_t bag_end   = (ins_i + 1 < inst_count) ? inst[ins_i+1].instBagNdx : ibag_count;
      for(uint16_t bag = bag_start; bag < bag_end && bag < ibag_count; ++bag)
      {
        uint16_t gen_start = ibag[bag].genIndex;
        uint16_t gen_end   = (bag + 1 < ibag_count) ? ibag[bag+1].genIndex : igen_count;
        uint16_t sample_id = 0xFFFF;
        uint8_t klow = 0, khigh = 127;
        int has_keyrange = 0;
        for(uint16_t g = gen_start; g < gen_end && g < igen_count; ++g)
        {
          uint16_t op = igen[g].genOper;
          int16_t amt = igen[g].genAmount;
          if(op == 53)
          {
            // sampleID
            sample_id = (uint16_t)amt;
          }
          else if(op == 43)
          {
            // keyRange (packed low/hi in a word)
            klow = (uint8_t)(amt & 0xFF);
            khigh = (uint8_t)((amt >> 8) & 0xFF);
            has_keyrange = 1;
          }
        }
        if(sample_id != 0xFFFF && sample_id < sample_count)
        {
          // assign keyrange if still default
          if(samples[sample_id].low_key == 0 && samples[sample_id].hi_key == 127)
          {
            samples[sample_id].low_key = has_keyrange ? klow : 0;
            samples[sample_id].hi_key  = has_keyrange ? khigh : 127;
          }
        }
      }
    }
  }

  /* --- Preset-level parsing: fill PresetEffects[] and optionally full generator table per preset --- */
  PresetEffects* preset_effects = NULL;
  int16_t** preset_gens = NULL;
  size_t preset_count = (phdr_count > 0) ? (phdr_count - 1) : 0; // exclude terminator

  if(phdr && pbag && pgen && preset_count > 0)
  {
    preset_effects = (PresetEffects*)calloc(preset_count, sizeof(PresetEffects));
    if(!preset_effects)
    { /* cleanup later */ }
  
    if(out_preset_generators)
    {
      // allocate matrix [preset_count][GEN_SLOTS]
      preset_gens = (int16_t**)malloc(sizeof(int16_t*) * preset_count);
      if (preset_gens)
      {
        for(size_t i = 0; i < preset_count; ++i)
        {
          preset_gens[i] = (int16_t*)calloc(GEN_SLOTS, sizeof(int16_t));
          if (!preset_gens[i])
          {
            // free previously allocated
            for (size_t j = 0; j < i; ++j) free(preset_gens[j]);
            free(preset_gens);
            preset_gens = NULL;
            break;
          }
        }
      }
    }
    
    for(size_t p = 0; p < preset_count; ++p)
    {
      PresetHeader* ph = &phdr[p];
      PresetEffects* pe = &preset_effects[p];
      
      // name
      memset(pe->name, 0, sizeof(pe->name));
      memcpy(pe->name, ph->presetName, 20);
      pe->preset_num = ph->preset;
      pe->bank_num = ph->bank;
      // defaults
      pe->pan = 0; pe->reverbSend = 0; pe->chorusSend = 0;
      pe->attack = pe->decay = pe->sustain = pe->release = 0;
      pe->cutoff = pe->resonance = 0;
      pe->seen_pan = pe->seen_reverb = pe->seen_chorus = 0;
      
      // default generator array zeros already by calloc
      int16_t* gens = (preset_gens) ? preset_gens[p] : NULL;
      
      // iterate bags for this preset
      uint16_t bag_start = ph->presetBagIndex;
      uint16_t bag_end = (p + 1 < phdr_count) ? phdr[p + 1].presetBagIndex : pbag_count;
      for(uint16_t bag = bag_start; bag < bag_end && bag < pbag_count; ++bag)
      {
        uint16_t gen_start = pbag[bag].genIndex;
        uint16_t gen_end   = (bag + 1 < pbag_count) ? pbag[bag+1].genIndex : pgen_count;
        for(uint16_t gi = gen_start; gi < gen_end && gi < pgen_count; ++gi)
        {
          uint16_t op = pgen[gi].genOper;
          int16_t amt = pgen[gi].genAmount;
          if(op < GEN_SLOTS && gens)
            gens[op] = amt;
          
          switch(op)
          {
            case 0x0A: pe->pan = amt; pe->seen_pan = 1; break;
            case 0x5B: pe->reverbSend = amt; pe->seen_reverb = 1; break;
            case 0x5C: pe->chorusSend = amt; pe->seen_chorus = 1; break;
            case 0x19: pe->attack = amt; break;
            case 0x1A: pe->decay = amt; break;
            case 0x1B: pe->sustain = amt; break;
            case 0x1C: pe->release = amt; break;
            case 0x23: pe->cutoff = amt; break;
            case 0x24: pe->resonance = amt; break;
            default: break;
          }
        }
      }
    }
  }

  /* --- prepare outputs --- */
  *out_samples = samples;
  *out_sample_count = (unsigned int)sample_count;
  if(out_presets)
    *out_presets = preset_effects;

  if(out_preset_count)
    *out_preset_count = (unsigned int)preset_count;

  if(out_preset_generators)
  *out_preset_generators = preset_gens;

    /* --- cleanup temporary resources --- */
    // We allocated and copied each sample's audio_data, so smpl_buf can be freed now
  if(smpl_buf)
  free(smpl_buf);
  
  // free PDTA arrays we no longer need
  if(phdr)
    free(phdr);
  
  if(pbag)
    free(pbag);
  
  if(pgen)
    free(pgen);
  
  if(inst)
    free(inst);
  
  if(ibag)
    free(ibag);
  
  if(igen)
    free(igen);
  
  if(shdr)
    free(shdr);
    

  fclose(f);
  return 0;
}