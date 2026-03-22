#pragma once

#include <stddef.h>

// Single cap for one-shot prefix read (JPEG SOF scan + shared sniff buffer).
constexpr size_t kImageDimensionsMaxPrefixBytes = 512 * 1024;

// Second read only when TIFF IFD is not in the first prefix (DNG / large headers).
constexpr size_t kImageDimensionsTiffMaxPrefixBytes = 8 * 1024 * 1024;

// WebP: EXIF/ICC/XMP chunks may precede VP8 / VP8L / VP8X by more than the default prefix.
constexpr size_t kImageDimensionsWebpMaxPrefixBytes = 8 * 1024 * 1024;
