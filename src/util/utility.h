#pragma once

#include <optional>

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