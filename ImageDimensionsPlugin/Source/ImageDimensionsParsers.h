#pragma once

#include <stddef.h>

// Buffer-only dimension parsers (no I/O). Used by ImageDimensionsReader.

bool TryParseJpegDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                            unsigned &outH, const volatile bool *pAbort);

bool TryParsePngDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                           unsigned &outH);

bool TryParseGifDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                           unsigned &outH);

bool TryParseBmpDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                           unsigned &outH);

bool TryParseIcoDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                           unsigned &outH);

bool TryParseWebpDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                            unsigned &outH);

bool TryParseTiffDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                          unsigned &outH);

bool TryParseSvgDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                           unsigned &outH);

bool TryParseJxlDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                           unsigned &outH);

// ISO BMFF container with ftyp major/compatible brand "jxl " (no leading JXL sig box).
bool IsJxlBmffFilePrefix(const unsigned char *buf, size_t len);
