#pragma once

#include <optional>
#include "enums.h"

template <typename MapType, typename KeyType>
std::optional<typename MapType::mapped_type> getValueFromMap(
    const MapType& map, const KeyType& key)
{
  auto it = map.find(key);
  if (it != map.end()) {
    return it->second;
  } else {
    return std::nullopt;
  }
}

uint32_t read6BitVariableLengthQuantity(const uint8_t* buffer, int maxLength, int& bytesRead);
void read7BitChunk(const uint8_t* encodedData, uint8_t* decodedData);

SynthType fileTypeToSynthType(SynthFileType fileType);