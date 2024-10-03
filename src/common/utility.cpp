#include "utility.h"

#include "../synths/Synth.h"
#include "enums.h"

#include <juce_core/system/juce_PlatformDefs.h>

uint32_t read6BitVariableLengthQuantity(const uint8_t* buffer, int maxLength, int& bytesRead) {
  uint32_t value = 0;
  bytesRead = 0;

  for (size_t i = 0; i < maxLength; ++i) {
    // Extract the current byte
    uint8_t currentByte = buffer[i];
    bytesRead++;

    // Mask off the most significant bit and add the byte to value
    value = (value << 6) | (currentByte & 0x3F);

    // Check if the most significant bit is not set (last byte of the variable quantity)
    if ((currentByte & 0x40) == 0) {
      break; // Last byte of the variable quantity value, stop processing
    }
  }
  return value;
}

void read7BitChunk(const uint8_t* encodedData, uint8_t* decodedData) {
  // Assuming encodedData points to 8 bytes of encoded data,
  // and decodedData is a buffer large enough to hold 7 bytes of decoded data.

  // Extract the 7 bits from the first byte and distribute them across the 7 bytes
  for (int i = 0; i < 7; ++i) {
    // Set the highest bit of each decoded byte based on the first encoded byte
    decodedData[i] = (encodedData[0] & (1 << i)) ? 0x80 : 0x00;
  }

  // Now handle the rest of the bytes
  for (int i = 1; i < 8; ++i) {
    // If we hit the sysex end before reading all 7 data bytes, we finish early. This should not occur.
    if (encodedData[i] == 0xF7) {
      jassert(true);
      break;
    }
    // Merge the lower 7 bits from each encoded byte into the decoded bytes
    decodedData[i - 1] |= encodedData[i] & 0x7F;
  }
}

SynthType fileTypeToSynthType(SynthFileType fileType) {
  switch (fileType) {
    case SynthFileType::SoundFont2: return SynthType::FluidSynth;
    // case SynthFileType::YM2
  }
}