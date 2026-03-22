#pragma once

// Extensible: add values and detection order in ImageDimensionsReader.cpp.
// Order matters for DetectImageFormat (see comments there).
enum class ImageFormat {
  Unknown,
  Png,
  Jxl,
  Jpeg,
  Gif,
  Webp,
  Tiff,
  Bmp,
  Ico,
  Svg
};

ImageFormat DetectImageFormat(const unsigned char *data, size_t len);

bool TryReadImageDimensions(const wchar_t *path, unsigned &outW, unsigned &outH,
                            const volatile bool *pAbort);
