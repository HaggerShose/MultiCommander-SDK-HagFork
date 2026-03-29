#pragma once

#include <cstddef>

namespace ImageDimensionsMagic {

extern const unsigned char kPngSignature[8];
extern const unsigned char kJxlContainerSignature[12];

constexpr size_t kPngSignatureSize = 8;
constexpr size_t kJxlContainerSignatureSize = 12;

} // namespace ImageDimensionsMagic
