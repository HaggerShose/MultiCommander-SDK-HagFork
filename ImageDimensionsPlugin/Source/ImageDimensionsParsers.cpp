#include "ImageDimensionsParsers.h"

#include "ImageDimensionsMagic.h"

#include <cstdint>
#include <cstring>
#include <limits>

namespace {

// --- Shared integer readers
// --------------------------------------------------------

uint16_t ReadU16LE(const unsigned char *p) {
  return static_cast<uint16_t>(static_cast<unsigned>(p[0]) |
                               (static_cast<unsigned>(p[1]) << 8));
}

uint16_t ReadU16BE(const unsigned char *p) {
  return static_cast<uint16_t>((static_cast<unsigned>(p[0]) << 8) |
                               static_cast<unsigned>(p[1]));
}

uint32_t ReadU32LE(const unsigned char *p) {
  return (static_cast<uint32_t>(p[0])) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

uint32_t ReadU32BE(const unsigned char *p) {
  return (static_cast<uint32_t>(p[0]) << 24) |
         (static_cast<uint32_t>(p[1]) << 16) |
         (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

// --- JPEG (SOF scan)
// ----------------------------------------------------------

bool IsSofMarker(unsigned char m) {
  return (m >= 0xC0 && m <= 0xC3) || (m >= 0xC5 && m <= 0xC7) ||
         (m >= 0xC9 && m <= 0xCB) || (m >= 0xCD && m <= 0xCF);
}

bool ParseJpegSofDimensions(const unsigned char *buf, size_t bufSize,
                            unsigned &outW, unsigned &outH,
                            const volatile bool *pAbort) {
  outW = 0;
  outH = 0;

  if (bufSize < 4 || buf[0] != 0xFF || buf[1] != 0xD8)
    return false;

  size_t pos = 2;
  while (pos + 1 < bufSize) {
    if (pAbort && *pAbort)
      return false;

    if (buf[pos] != 0xFF) {
      ++pos;
      continue;
    }

    size_t m = pos + 1;
    while (m < bufSize && buf[m] == 0xFF)
      ++m;
    if (m >= bufSize)
      break;

    const unsigned char mk = buf[m];
    if (mk == 0x00 || mk == 0xFF) {
      pos = m + 1;
      continue;
    }

    if (mk >= 0xD0 && mk <= 0xD7) {
      pos = m + 1;
      continue;
    }
    if (mk == 0xD8) {
      pos = m + 1;
      continue;
    }
    if (mk == 0xD9)
      break;

    if (m + 3 > bufSize)
      break;

    const unsigned segLen = (static_cast<unsigned>(buf[m + 1]) << 8) |
                            static_cast<unsigned>(buf[m + 2]);
    if (segLen < 2)
      return false;

    if (m + 1 + segLen > bufSize)
      break;

    if (IsSofMarker(mk)) {
      if (segLen < 8) {
        pos = m + 1 + segLen;
        continue;
      }
      const unsigned h = (static_cast<unsigned>(buf[m + 4]) << 8) |
                         static_cast<unsigned>(buf[m + 5]);
      const unsigned w = (static_cast<unsigned>(buf[m + 6]) << 8) |
                         static_cast<unsigned>(buf[m + 7]);
      if (w > 0 && h > 0) {
        outW = w;
        outH = h;
        return true;
      }
      pos = m + 1 + segLen;
      continue;
    }

    pos = m + 1 + segLen;
  }

  return false;
}

// --- PNG (IHDR)
// ---------------------------------------------------------------

constexpr size_t kPngIhdrBytes = 8 + 4 + 4 + 13;

bool ParsePngIhdr(const unsigned char *buf, size_t n, unsigned &outW,
                  unsigned &outH) {
  outW = 0;
  outH = 0;
  if (n < kPngIhdrBytes)
    return false;
  if (memcmp(buf, ImageDimensionsMagic::kPngSignature,
             ImageDimensionsMagic::kPngSignatureSize) != 0)
    return false;
  const uint32_t chunkLen = ReadU32BE(buf + 8);
  if (chunkLen != 13)
    return false;
  if (buf[12] != 'I' || buf[13] != 'H' || buf[14] != 'D' || buf[15] != 'R')
    return false;
  const uint32_t w = ReadU32BE(buf + 16);
  const uint32_t h = ReadU32BE(buf + 20);
  if (w == 0 || h == 0)
    return false;
  outW = w;
  outH = h;
  return true;
}

// --- GIF (logical screen)
// -----------------------------------------------------

constexpr size_t kGifLogicalScreenDescriptorEnd = 10;

bool IsGif87aOr89a(const unsigned char *buf, size_t n) {
  if (n < 6)
    return false;
  return buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F' && buf[3] == '8' &&
         (buf[4] == '7' || buf[4] == '9') && buf[5] == 'a';
}

// --- WebP
// ------------------------------------------------------------

bool ParseWebpVp8LossyPayload(const unsigned char *payload, size_t psz,
                              unsigned &outW, unsigned &outH) {
  outW = 0;
  outH = 0;
  // VP8 keyframe: 3-byte frame tag, start code 9D 01 2A @3..5, width/height
  // @6..9 (LE uint16, 14-bit each — libwebp / RFC 6386).
  if (psz < 10)
    return false;
  if (payload[3] != 0x9D || payload[4] != 0x01 || payload[5] != 0x2A)
    return false;
  const unsigned w = static_cast<unsigned>(ReadU16LE(payload + 6) & 0x3FFFu);
  const unsigned h = static_cast<unsigned>(ReadU16LE(payload + 8) & 0x3FFFu);
  if (w == 0 || h == 0)
    return false;
  outW = w;
  outH = h;
  return true;
}

bool ParseWebpVp8lPayload(const unsigned char *payload, size_t psz,
                          unsigned &outW, unsigned &outH) {
  outW = 0;
  outH = 0;
  if (psz < 5 || payload[0] != 0x2F)
    return false;
  const uint32_t bits = ReadU32LE(payload + 1);
  const unsigned w = (bits & 0x3FFFu) + 1u;
  const unsigned h = ((bits >> 14) & 0x3FFFu) + 1u;
  if (w == 0 || h == 0)
    return false;
  outW = w;
  outH = h;
  return true;
}

bool ParseWebpVp8xPayload(const unsigned char *payload, size_t psz,
                          unsigned &outW, unsigned &outH) {
  outW = 0;
  outH = 0;
  // VP8X chunk payload (RIFF spec): byte 0 = feature flags; bytes 1-3 =
  // reserved (0); bytes 4-6 = canvas width minus one; bytes 7-9 = canvas height
  // minus one (24-bit LE each).
  if (psz < 10)
    return false;
  const unsigned w = 1u + (static_cast<unsigned>(payload[4]) |
                           (static_cast<unsigned>(payload[5]) << 8) |
                           (static_cast<unsigned>(payload[6]) << 16));
  const unsigned h = 1u + (static_cast<unsigned>(payload[7]) |
                           (static_cast<unsigned>(payload[8]) << 8) |
                           (static_cast<unsigned>(payload[9]) << 16));
  if (w == 0 || h == 0)
    return false;
  outW = w;
  outH = h;
  return true;
}

// One RIFF-like region: top-level WebP payload after the 12-byte file header,
// or ANMF frame data. Advance by declared chunk size even if the payload is not
// fully in `len`; parsers only receive min(declared, available) bytes.
static bool WebpScanChunkRegion(const unsigned char *base, size_t len,
                                unsigned &outW, unsigned &outH) {
  outW = 0;
  outH = 0;
  size_t off = 0;
  while (off + 8 <= len) {
    const unsigned char *ch = base + off;
    const uint32_t declared = ReadU32LE(ch + 4);
    const size_t payloadOff = off + 8;
    const size_t avail = len - payloadOff;
    const size_t psz = declared < avail ? declared : avail;

    if (memcmp(ch, "VP8 ", 4) == 0) {
      if (ParseWebpVp8LossyPayload(base + payloadOff, psz, outW, outH))
        return true;
    } else if (memcmp(ch, "VP8L", 4) == 0) {
      if (ParseWebpVp8lPayload(base + payloadOff, psz, outW, outH))
        return true;
    } else if (memcmp(ch, "VP8X", 4) == 0) {
      if (ParseWebpVp8xPayload(base + payloadOff, psz, outW, outH))
        return true;
    } else if (memcmp(ch, "ANMF", 4) == 0 && declared >= 16) {
      const size_t frameDeclared = declared - 16;
      const size_t frameAvail = avail > 16 ? avail - 16 : 0;
      const size_t innerLen =
          frameDeclared < frameAvail ? frameDeclared : frameAvail;
      const unsigned char *frame = base + payloadOff + 16;
      if (innerLen > 0 && WebpScanChunkRegion(frame, innerLen, outW, outH))
        return true;
    }

    const size_t pad = static_cast<size_t>(declared & 1u);
    const uint64_t nextU = static_cast<uint64_t>(payloadOff) +
                           static_cast<uint64_t>(declared) +
                           static_cast<uint64_t>(pad);
    const uint64_t maxOff =
        static_cast<uint64_t>((std::numeric_limits<size_t>::max)());
    if (nextU > maxOff || nextU <= static_cast<uint64_t>(off))
      break;
    off = static_cast<size_t>(nextU);
  }
  return false;
}

// --- TIFF (first IFD, tags 256 / 257)
// ------------------------------------------------------------

bool TiffReadTagValue(uint16_t type, uint32_t count, uint32_t valueField,
                      uint32_t &outVal) {
  outVal = 0;
  if (count != 1)
    return false;
  if (type == 3) {
    outVal = valueField & 0xFFFFu;
    return outVal > 0;
  }
  if (type == 4) {
    outVal = valueField;
    return outVal > 0;
  }
  return false;
}

// --- JPEG XL (codestream size header, LSB-first bit reader; after FFmpeg)
// ------------------------------------------------------------

struct JxlBitReaderLe {
  const unsigned char *p;
  size_t lenBytes;
  size_t bitPos = 0;
  bool err = false;

  bool haveBits(size_t n) const { return bitPos + n <= lenBytes * 8; }

  unsigned pull(unsigned n) {
    if (!haveBits(n)) {
      err = true;
      return 0;
    }
    unsigned v = 0;
    for (unsigned i = 0; i < n; ++i) {
      const size_t b = bitPos / 8;
      const int o = static_cast<int>(bitPos % 8);
      ++bitPos;
      if ((p[b] >> o) & 1)
        v |= (1u << i);
    }
    return v;
  }
};

static uint32_t JxlU32(JxlBitReaderLe &gb, uint32_t c0, uint32_t c1,
                       uint32_t c2, uint32_t c3, uint32_t u0, uint32_t u1,
                       uint32_t u2, uint32_t u3) {
  const uint32_t choice = gb.pull(2);
  const uint32_t c[] = {c0, c1, c2, c3};
  const uint32_t u[] = {u0, u1, u2, u3};
  uint32_t ret = c[choice];
  if (u[choice] != 0)
    ret += gb.pull(u[choice]);
  return ret;
}

static uint32_t JxlWidthFromRatio(uint32_t height, int ratio) {
  const uint64_t h64 = height;
  switch (ratio) {
  case 1:
    return height;
  case 2:
    return static_cast<uint32_t>((h64 * 12) / 10);
  case 3:
    return static_cast<uint32_t>((h64 * 4) / 3);
  case 4:
    return static_cast<uint32_t>((h64 * 3) / 2);
  case 5:
    return static_cast<uint32_t>((h64 * 16) / 9);
  case 6:
    return static_cast<uint32_t>((h64 * 5) / 4);
  case 7:
    return static_cast<uint32_t>(h64 * 2);
  default:
    return 0;
  }
}

static bool JxlReadSizeHeader(JxlBitReaderLe &gb, unsigned &outW,
                              unsigned &outH) {
  uint32_t width = 0;
  uint32_t height = 0;
  if (gb.pull(1)) {
    height = (gb.pull(5) + 1u) << 3u;
    const int ratio = static_cast<int>(gb.pull(3));
    width = JxlWidthFromRatio(height, ratio);
    if (width == 0)
      width = (gb.pull(5) + 1u) << 3u;
  } else {
    height = 1 + JxlU32(gb, 0, 0, 0, 0, 9, 13, 18, 30);
    const int ratio = static_cast<int>(gb.pull(3));
    width = JxlWidthFromRatio(height, ratio);
    if (width == 0)
      width = 1 + JxlU32(gb, 0, 0, 0, 0, 9, 13, 18, 30);
  }
  if (gb.err || width == 0 || height == 0 || width > (1u << 18) ||
      height > (1u << 18))
    return false;
  outW = width;
  outH = height;
  return true;
}

static bool JxlParseCodestream(const unsigned char *b, size_t n, unsigned &outW,
                               unsigned &outH) {
  outW = outH = 0;
  if (n < 4)
    return false;
  JxlBitReaderLe gb{b, n, 0};
  const uint32_t sig = gb.pull(16);
  if (sig != 0x0AFFu)
    return false;
  return JxlReadSizeHeader(gb, outW, outH) && !gb.err;
}

// ftyp tag as ReadU32LE("ftyp")
static constexpr uint32_t kBmffFtypTag = 0x70797466u;
// jxlc / jxlp tags as ReadU32LE
static constexpr uint32_t kJxlBoxJxlc = 0x636C786Au;
static constexpr uint32_t kJxlBoxJxlp = 0x706C786Au;

static bool FtypPayloadDeclaresJxlBrand(const unsigned char *payload,
                                        size_t psz) {
  if (psz < 8)
    return false;
  if (memcmp(payload, "jxl ", 4) == 0)
    return true;
  for (size_t i = 8; i + 4 <= psz; i += 4) {
    if (memcmp(payload + i, "jxl ", 4) == 0)
      return true;
  }
  return false;
}

static bool IsJxlFtypLeadingBox(const unsigned char *buf, size_t len) {
  if (len < 16)
    return false;
  uint64_t boxSize = ReadU32BE(buf);
  size_t head = 8;
  if (boxSize == 1) {
    boxSize = 0;
    for (int i = 0; i < 8; ++i)
      boxSize = (boxSize << 8) | buf[8 + static_cast<size_t>(i)];
    head = 16;
  }
  if (boxSize != 0 && boxSize < head)
    return false;
  const uint32_t tag = ReadU32LE(buf + 4);
  if (tag != kBmffFtypTag)
    return false;
  const size_t payloadOff = head;
  if (payloadOff > len)
    return false;
  size_t psz = 0;
  if (boxSize == 0)
    psz = len > payloadOff ? len - payloadOff : 0;
  else
    psz = static_cast<size_t>(boxSize - head);
  if (psz < 8 || psz > len - payloadOff)
    return false;
  return FtypPayloadDeclaresJxlBrand(buf + payloadOff, psz);
}

static bool JxlWalkBoxesFindDimensions(const unsigned char *buf, size_t len,
                                       unsigned &outW, unsigned &outH) {
  size_t o = 0;
  while (o + 8 <= len) {
    uint64_t boxSize = ReadU32BE(buf + o);
    const uint32_t tag = ReadU32LE(buf + o + 4);
    size_t head = 8;
    if (boxSize == 1) {
      if (o + 16 > len)
        break;
      boxSize = 0;
      for (int i = 0; i < 8; ++i)
        boxSize = (boxSize << 8) | buf[o + 8 + static_cast<size_t>(i)];
      head = 16;
    }
    if (boxSize != 0 && boxSize < head)
      break;
    const size_t payload = o + head;
    size_t psz = 0;
    if (boxSize == 0)
      psz = len > payload ? len - payload : 0;
    else
      psz = static_cast<size_t>(boxSize - head);
    if (payload > len || psz > len - payload)
      break;
    if (tag == kJxlBoxJxlc) {
      if (JxlParseCodestream(buf + payload, psz, outW, outH))
        return true;
    } else if (tag == kJxlBoxJxlp) {
      if (psz >= 4 &&
          JxlParseCodestream(buf + payload + 4, psz - 4, outW, outH))
        return true;
    }
    if (boxSize == 0)
      break;
    o = payload + psz;
  }
  return false;
}

} // namespace

bool IsJxlBmffFilePrefix(const unsigned char *buf, size_t len) {
  return IsJxlFtypLeadingBox(buf, len);
}

bool TryParseJpegDimensions(const unsigned char *buf, size_t len,
                            unsigned &outW, unsigned &outH,
                            const volatile bool *pAbort) {
  return ParseJpegSofDimensions(buf, len, outW, outH, pAbort);
}

bool TryParsePngDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                           unsigned &outH) {
  return ParsePngIhdr(buf, len, outW, outH);
}

bool TryParseGifDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                           unsigned &outH) {
  outW = 0;
  outH = 0;
  if (len < kGifLogicalScreenDescriptorEnd)
    return false;
  if (!IsGif87aOr89a(buf, len))
    return false;
  const unsigned w = ReadU16LE(buf + 6);
  const unsigned h = ReadU16LE(buf + 8);
  if (w == 0 || h == 0)
    return false;
  outW = w;
  outH = h;
  return true;
}

bool TryParseBmpDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                           unsigned &outH) {
  outW = 0;
  outH = 0;
  if (len < 26)
    return false;
  if (buf[0] != 'B' || buf[1] != 'M')
    return false;
  const uint32_t biSize = ReadU32LE(buf + 14);
  if (14 + biSize > len)
    return false;
  if (biSize == 12) {
    if (len < 26)
      return false;
    const unsigned w = ReadU16LE(buf + 18);
    const int16_t hRaw = static_cast<int16_t>(ReadU16LE(buf + 20));
    if (w == 0)
      return false;
    const unsigned h = hRaw < 0 ? static_cast<unsigned>(-static_cast<int>(hRaw))
                                : static_cast<unsigned>(hRaw);
    if (h == 0)
      return false;
    outW = w;
    outH = h;
    return true;
  }
  if (biSize < 40 || 14 + biSize > len)
    return false;
  const int32_t w32 = static_cast<int32_t>(ReadU32LE(buf + 18));
  const int32_t h32 = static_cast<int32_t>(ReadU32LE(buf + 22));
  if (w32 <= 0)
    return false;
  const unsigned h = h32 < 0 ? static_cast<unsigned>(-static_cast<int64_t>(h32))
                             : static_cast<unsigned>(h32);
  if (h == 0)
    return false;
  outW = static_cast<unsigned>(w32);
  outH = h;
  return true;
}

bool TryParseWebpDimensions(const unsigned char *buf, size_t len,
                            unsigned &outW, unsigned &outH) {
  outW = 0;
  outH = 0;
  if (len < 12)
    return false;
  if (buf[0] != 'R' || buf[1] != 'I' || buf[2] != 'F' || buf[3] != 'F')
    return false;
  if (buf[8] != 'W' || buf[9] != 'E' || buf[10] != 'B' || buf[11] != 'P')
    return false;
  return WebpScanChunkRegion(buf + 12, len - 12, outW, outH);
}

bool TryParseTiffDimensions(const unsigned char *buf, size_t len,
                            unsigned &outW, unsigned &outH) {
  outW = 0;
  outH = 0;
  if (len < 8)
    return false;
  bool be;
  if (buf[0] == 'I' && buf[1] == 'I')
    be = false;
  else if (buf[0] == 'M' && buf[1] == 'M')
    be = true;
  else
    return false;
  const uint16_t magic42 = be ? ReadU16BE(buf + 2) : ReadU16LE(buf + 2);
  if (magic42 != 42)
    return false;
  const uint32_t ifd0 = be ? ReadU32BE(buf + 4) : ReadU32LE(buf + 4);
  if (ifd0 > len || ifd0 + 2 > len)
    return false;
  const uint16_t nents = be ? ReadU16BE(buf + ifd0) : ReadU16LE(buf + ifd0);
  const size_t dirStart = ifd0 + 2;
  if (nents > 4096 || dirStart + static_cast<size_t>(nents) * 12 > len)
    return false;
  uint32_t w = 0;
  uint32_t h = 0;
  for (uint16_t i = 0; i < nents; ++i) {
    const unsigned char *e = buf + dirStart + static_cast<size_t>(i) * 12;
    const uint16_t tag = be ? ReadU16BE(e) : ReadU16LE(e);
    const uint16_t typ = be ? ReadU16BE(e + 2) : ReadU16LE(e + 2);
    const uint32_t cnt = be ? ReadU32BE(e + 4) : ReadU32LE(e + 4);
    const uint32_t vf = be ? ReadU32BE(e + 8) : ReadU32LE(e + 8);
    uint32_t v = 0;
    if (tag == 256 && TiffReadTagValue(typ, cnt, vf, v))
      w = v;
    if (tag == 257 && TiffReadTagValue(typ, cnt, vf, v))
      h = v;
  }
  if (w == 0 || h == 0)
    return false;
  outW = w;
  outH = h;
  return true;
}

bool TryParseJxlDimensions(const unsigned char *buf, size_t len, unsigned &outW,
                           unsigned &outH) {
  outW = 0;
  outH = 0;
  const bool hasJxlSigBox =
      len >= ImageDimensionsMagic::kJxlContainerSignatureSize &&
      memcmp(buf, ImageDimensionsMagic::kJxlContainerSignature,
             ImageDimensionsMagic::kJxlContainerSignatureSize) == 0;
  if (hasJxlSigBox || IsJxlFtypLeadingBox(buf, len))
    return JxlWalkBoxesFindDimensions(buf, len, outW, outH);
  return JxlParseCodestream(buf, len, outW, outH);
}
