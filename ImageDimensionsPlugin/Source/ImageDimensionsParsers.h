#pragma once

#include <stddef.h>

enum class ImageFormat { Unknown, Png, Jxl, Jpeg, Gif, Webp, Tiff, Bmp };

// Order matters (see DetectImageFormat).
ImageFormat DetectImageFormat(const unsigned char *data, size_t len);

bool ParseImageDimensions(ImageFormat fmt, const unsigned char *buf, size_t len,
                          unsigned &outW, unsigned &outH,
                          const volatile bool *pAbort);
