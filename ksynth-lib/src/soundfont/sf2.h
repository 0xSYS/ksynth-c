#ifndef SF2_H
#define SF2_H

#include <stdint.h>

#include "../utils.h"


typedef struct
{
  char id[4];
  uint32_t size;
}ChunkHeader;

typedef struct
{
  char id[4];
  uint32_t size;
  char type[4];
}RiffChunkHeader;

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






#endif