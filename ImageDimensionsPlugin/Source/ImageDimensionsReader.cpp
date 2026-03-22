#include "PluginWinConfig.h"

#include "ImageDimensionsReader.h"

#include "ImageDimensionsLimits.h"
#include "ImageDimensionsParsers.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {
static const unsigned char kPngSignature[8] = {0x89, 0x50, 0x4E, 0x47,
                                               0x0D, 0x0A, 0x1A, 0x0A};

static const unsigned char kJxlFileSig[12] = {0,   0,   0,    0x0C, 'J',  'X',
                                              'L', ' ', 0x0D, 0x0A, 0x87, 0x0A};

bool HasPngSignature(const unsigned char *data, size_t len) {
  if (len < 8)
    return false;
  return memcmp(data, kPngSignature, 8) == 0;
}

bool HasJxlContainerSignature(const unsigned char *data, size_t len) {
  return len >= 12 && memcmp(data, kJxlFileSig, 12) == 0;
}

bool HasJxlCodestreamSignature(const unsigned char *data, size_t len) {
  return len >= 2 && data[0] == 0xFF && data[1] == 0x0A;
}

bool HasJpegSignature(const unsigned char *data, size_t len) {
  return len >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF;
}

bool HasGifSignature(const unsigned char *data, size_t len) {
  return len >= 6 && data[0] == 'G' && data[1] == 'I' && data[2] == 'F' &&
         data[3] == '8' && (data[4] == '7' || data[4] == '9') && data[5] == 'a';
}

bool HasWebpSignature(const unsigned char *data, size_t len) {
  return len >= 12 && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' &&
         data[3] == 'F' && data[8] == 'W' && data[9] == 'E' &&
         data[10] == 'B' && data[11] == 'P';
}

bool HasTiffSignature(const unsigned char *data, size_t len) {
  if (len < 4)
    return false;
  return (data[0] == 'I' && data[1] == 'I' && data[2] == '*' && data[3] == 0) ||
         (data[0] == 'M' && data[1] == 'M' && data[2] == 0 && data[3] == '*');
}

bool HasBmpSignature(const unsigned char *data, size_t len) {
  return len >= 2 && data[0] == 'B' && data[1] == 'M';
}

bool HasIcoSignature(const unsigned char *data, size_t len) {
  return len >= 6 && data[0] == 0 && data[1] == 0 && data[2] == 1 &&
         data[3] == 0;
}

bool HasSvgHeuristic(const unsigned char *data, size_t len) {
  if (len < 5)
    return false;
  size_t i = 0;
  if (len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
    i = 3;
  while (i < len && (data[i] == ' ' || data[i] == '\t' || data[i] == '\r' ||
                     data[i] == '\n'))
    ++i;
  const size_t winEnd = i + 256 < len ? i + 256 : len;
  for (size_t j = i; j + 4 < winEnd; ++j) {
    if (data[j] != '<')
      continue;
    if ((data[j + 1] == 's' || data[j + 1] == 'S') &&
        (data[j + 2] == 'v' || data[j + 2] == 'V') &&
        (data[j + 3] == 'g' || data[j + 3] == 'G'))
      return true;
    if (data[j + 1] == '?' && (data[j + 2] == 'x' || data[j + 2] == 'X') &&
        (data[j + 3] == 'm' || data[j + 3] == 'M'))
      return true;
  }
  return false;
}

bool ParseByFormat(ImageFormat fmt, const unsigned char *p, size_t n,
                   unsigned &outW, unsigned &outH,
                   const volatile bool *pAbort) {
  switch (fmt) {
  case ImageFormat::Png:
    return TryParsePngDimensions(p, n, outW, outH);
  case ImageFormat::Jxl:
    return TryParseJxlDimensions(p, n, outW, outH);
  case ImageFormat::Jpeg:
    return TryParseJpegDimensions(p, n, outW, outH, pAbort);
  case ImageFormat::Gif:
    return TryParseGifDimensions(p, n, outW, outH);
  case ImageFormat::Webp:
    return TryParseWebpDimensions(p, n, outW, outH);
  case ImageFormat::Tiff:
    return TryParseTiffDimensions(p, n, outW, outH);
  case ImageFormat::Bmp:
    return TryParseBmpDimensions(p, n, outW, outH);
  case ImageFormat::Ico:
    return TryParseIcoDimensions(p, n, outW, outH);
  case ImageFormat::Svg:
    return TryParseSvgDimensions(p, n, outW, outH);
  default:
    return false;
  }
}
} // namespace

ImageFormat DetectImageFormat(const unsigned char *data, size_t len) {
  // PNG: unique 0x89 prefix.
  if (HasPngSignature(data, len))
    return ImageFormat::Png;
  // JPEG XL: container or raw codestream (FF 0A) before generic JPEG (FF D8).
  if (HasJxlContainerSignature(data, len) ||
      HasJxlCodestreamSignature(data, len))
    return ImageFormat::Jxl;
  if (HasJpegSignature(data, len))
    return ImageFormat::Jpeg;
  if (HasGifSignature(data, len))
    return ImageFormat::Gif;
  // WebP: RIFF .... WEBP (before other RIFF types).
  if (HasWebpSignature(data, len))
    return ImageFormat::Webp;
  if (HasTiffSignature(data, len))
    return ImageFormat::Tiff;
  if (HasBmpSignature(data, len))
    return ImageFormat::Bmp;
  if (HasIcoSignature(data, len))
    return ImageFormat::Ico;
  if (HasSvgHeuristic(data, len))
    return ImageFormat::Svg;
  return ImageFormat::Unknown;
}

bool TryReadImageDimensions(const wchar_t *path, unsigned &outW, unsigned &outH,
                            const volatile bool *pAbort) {
  outW = 0;
  outH = 0;

  if (path == nullptr)
    return false;
  if (pAbort && *pAbort)
    return false;

  HANDLE h =
      CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                  nullptr, OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
  if (h == INVALID_HANDLE_VALUE)
    return false;

  LARGE_INTEGER fileSize{};
  if (!GetFileSizeEx(h, &fileSize) || fileSize.QuadPart < 2) {
    CloseHandle(h);
    return false;
  }

  const uint64_t sz = static_cast<uint64_t>(fileSize.QuadPart);

  auto readPrefix = [&](size_t maxBytes,
                        std::vector<unsigned char> &buf) -> bool {
    const size_t toRead = sz < maxBytes ? static_cast<size_t>(sz) : maxBytes;
    buf.resize(toRead);
    DWORD rd = 0;
    if (SetFilePointer(h, 0, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER &&
        GetLastError() != NO_ERROR)
      return false;
    if (!ReadFile(h, buf.data(), static_cast<DWORD>(toRead), &rd, nullptr) ||
        rd < 2)
      return false;
    buf.resize(rd);
    return true;
  };

  std::vector<unsigned char> buf;
  if (!readPrefix(kImageDimensionsMaxPrefixBytes, buf)) {
    CloseHandle(h);
    return false;
  }

  if (pAbort && *pAbort) {
    CloseHandle(h);
    return false;
  }

  const unsigned char *p = buf.data();
  const size_t n = buf.size();
  ImageFormat fmt = DetectImageFormat(p, n);

  if (ParseByFormat(fmt, p, n, outW, outH, pAbort)) {
    CloseHandle(h);
    return true;
  }

  if (fmt == ImageFormat::Tiff && sz > n &&
      n < kImageDimensionsTiffMaxPrefixBytes) {
    if (pAbort && *pAbort) {
      CloseHandle(h);
      return false;
    }
    if (!readPrefix(kImageDimensionsTiffMaxPrefixBytes, buf)) {
      CloseHandle(h);
      return false;
    }
    if (pAbort && *pAbort) {
      CloseHandle(h);
      return false;
    }
    const bool ok = TryParseTiffDimensions(buf.data(), buf.size(), outW, outH);
    CloseHandle(h);
    return ok;
  }

  if (fmt == ImageFormat::Webp && sz > n &&
      n < kImageDimensionsWebpMaxPrefixBytes) {
    if (pAbort && *pAbort) {
      CloseHandle(h);
      return false;
    }
    if (!readPrefix(kImageDimensionsWebpMaxPrefixBytes, buf)) {
      CloseHandle(h);
      return false;
    }
    if (pAbort && *pAbort) {
      CloseHandle(h);
      return false;
    }
    const bool ok = TryParseWebpDimensions(buf.data(), buf.size(), outW, outH);
    CloseHandle(h);
    return ok;
  }

  CloseHandle(h);
  return false;
}
