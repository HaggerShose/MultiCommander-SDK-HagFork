#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ImageDimensionsReader.h"

#include "ImageDimensionsParsers.h"

#include <cstdint>
#include <vector>

namespace {
// Read only as much as needed: start small, double until parse works.
// Avoids the old 32 KB -> 512 KB jump (almost every camera JPEG paid 512 KB).
constexpr size_t kFirstReadBytes = 64 * 1024;
constexpr size_t kMaxHeaderBytes = 8 * 1024 * 1024;

bool NeedsDeepHeader(ImageFormat fmt) {
  return fmt == ImageFormat::Jpeg || fmt == ImageFormat::Tiff ||
         fmt == ImageFormat::Webp;
}
} // namespace

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
  size_t target = kFirstReadBytes;
  if (target > sz)
    target = static_cast<size_t>(sz);

  std::vector<unsigned char> buf;
  buf.reserve(target < kFirstReadBytes * 2 ? kFirstReadBytes * 2 : target);

  ImageFormat fmt = ImageFormat::Unknown;
  bool first = true;

  for (;;) {
    if (pAbort && *pAbort) {
      CloseHandle(h);
      return false;
    }

    const size_t old = buf.size();
    if (target <= old)
      break;

    const size_t want = target - old;
    buf.resize(target);
    DWORD rd = 0;
    if (!ReadFile(h, buf.data() + old, static_cast<DWORD>(want), &rd,
                  nullptr) ||
        rd == 0) {
      buf.resize(old);
      if (old == 0) {
        CloseHandle(h);
        return false;
      }
      break;
    }
    if (old == 0 && rd < 2) {
      CloseHandle(h);
      return false;
    }
    buf.resize(old + rd);

    if (first) {
      fmt = DetectImageFormat(buf.data(), buf.size());
      first = false;
    }

    if (ParseImageDimensions(fmt, buf.data(), buf.size(), outW, outH, pAbort)) {
      CloseHandle(h);
      return true;
    }

    if (!NeedsDeepHeader(fmt) || buf.size() >= sz ||
        buf.size() >= kMaxHeaderBytes)
      break;

    // Double cap for next append (e.g. 64 -> 128 -> 256 ...).
    if (target >= kMaxHeaderBytes)
      break;
    target *= 2;
    if (target > kMaxHeaderBytes)
      target = kMaxHeaderBytes;
    if (target > sz)
      target = static_cast<size_t>(sz);
  }

  CloseHandle(h);
  return false;
}
