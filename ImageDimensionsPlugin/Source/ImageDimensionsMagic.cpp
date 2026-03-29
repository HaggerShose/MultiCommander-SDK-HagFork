#include "ImageDimensionsMagic.h"

namespace ImageDimensionsMagic {

const unsigned char kPngSignature[8] = {0x89, 0x50, 0x4E, 0x47,
                                        0x0D, 0x0A, 0x1A, 0x0A};

const unsigned char kJxlContainerSignature[12] = {0,   0,   0,    0x0C, 'J',
                                                  'X', 'L', ' ',  0x0D, 0x0A,
                                                  0x87, 0x0A};

} // namespace ImageDimensionsMagic
