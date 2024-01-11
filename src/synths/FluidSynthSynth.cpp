#include <iterator>
#include <fluidsynth.h>
#include "FluidSynthSynth.h"
#include "Util.h"

constexpr int NUM_CHANNELS = 48;

#ifdef _WIN32
#define FLUID_PRIi64 "I64d"
#else
#define FLUID_PRIi64 "lld"
#endif

fluid_long_long_t fileOffset;
fluid_long_long_t fileSize;

void *mem_open(const char *path)
{
  void *buffer;

  if(path[0] != '&')
  {
    return nullptr;
  }
  sscanf(path, "&%p %" FLUID_PRIi64, &buffer, &fileSize);
  fileOffset = 0;
  return buffer;
}

int mem_read(void *buf, fluid_long_long_t count, void * data)
{
  memcpy(buf, ((uint8_t*)data) + fileOffset, (size_t)count);
  fileOffset += count;
  return FLUID_OK;
}

int mem_seek(void * data, fluid_long_long_t offset, int whence)
{
  if (whence == SEEK_SET) {
    fileOffset = offset;
  } else if (whence == SEEK_CUR) {
    fileOffset += offset;
  }  else if (whence == SEEK_END) {
    fileOffset = fileSize + offset;
  }
  return FLUID_OK;
}

int mem_close(void *handle)
{
  fileOffset = 0;
  fileSize = 0;
  return FLUID_OK;
}

fluid_long_long_t mem_tell(void *handle)
{
  return fileOffset;
}

FluidSynthSynth::FluidSynthSynth(AudioProcessorValueTreeState& vst)
    : valueTreeState(vst) {

}