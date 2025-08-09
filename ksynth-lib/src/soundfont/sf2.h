#ifndef SF2_H
#define SF2_H

#include <stdint.h>

#include "../core/sample.h"
#include "../utils.h"

#define MAX_SAMPLES 5500
#define GEN_KEYRANGE 43
#define GEN_SLOTS 128




#pragma pack(push, 1)
typedef struct
{
  char id[4];
  uint32_t size;
}ChunkHeader;
#pragma pack(pop)

typedef struct
{
  char id[4];
  uint32_t size;
  char type[4];
}RiffChunkHeader;

// phdr
#pragma pack(push, 1)
typedef struct
{
  char presetName[20];     // 20 bytes
  uint16_t preset;         // 2 bytes
  uint16_t bank;           // 2 bytes
  uint16_t presetBagIndex; // 2 bytes
  uint32_t library;        // 4 bytes
  uint32_t genre;          // 4 bytes
  uint32_t morphology;     // 4 bytes
}PresetHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
  char sampleName[20];
  uint32_t start;
  uint32_t end;
  uint32_t startLoop;
  uint32_t endLoop;
  uint32_t sampleRate;
  uint8_t originalPitch;
  int8_t pitchCorrection;
  uint16_t sampleLink;
  uint16_t sampleType;
}SampleHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
  char instName[20];
  uint16_t instBagNdx;
}Instrument;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
  uint16_t genIndex;
  uint16_t modIndex;
}InstBag;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
  uint16_t genOper;
  int16_t genAmount;
}InstGen;
#pragma pack(pop)

/* PresetEffects is kept as provided (packed) */
#pragma pack(push, 1)
typedef struct
{
  char name[21];
  uint16_t preset_num;
  uint16_t bank_num;

  int16_t pan;          // gen 0x0A
  int16_t reverbSend;   // gen 0x5B (91)
  int16_t chorusSend;   // gen 0x5C (92)

  int16_t attack;       // gen 0x19
  int16_t decay;        // gen 0x1A
  int16_t sustain;      // gen 0x1B
  int16_t release;      // gen 0x1C

  int16_t cutoff;       // gen 0x23
  int16_t resonance;    // gen 0x24

  int seen_pan;
  int seen_reverb;
  int seen_chorus;
} PresetEffects;
#pragma pack(pop)






KSYNTH_API int ksynth_sf2_load(const char* path, struct Sample** out_samples, unsigned int* out_sample_count, PresetEffects** out_presets, unsigned int* out_preset_count, int16_t*** out_preset_generators);



#endif