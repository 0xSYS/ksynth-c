#ifndef SF2_H
#define SF2_H

#include <stdint.h>

#include "../utils.h"

#define MAX_SAMPLES 5500
#define GEN_KEYRANGE 43


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

// Apparently packing up structs is cucial when storing sf2 data :eyes:
#pragma pack(push, 1)
typedef struct
{
  char presetName[20];
  uint16_t preset;
  uint16_t bank;
  uint16_t presetBagIndex;
  uint32_t library;
  uint32_t genre;
  uint32_t morphology;
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







#endif