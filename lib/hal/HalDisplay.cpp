#include <HalDisplay.h>

#include <Logging.h>

#include <algorithm>
#include <cassert>
#include <cstring>

HalDisplay display;

namespace {
constexpr uint8_t kBayer4x4[4][4] = {
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5},
};

void deselectSharedSpiDevices() {
  pinMode(BOARD_LORA_CS, OUTPUT);
  digitalWrite(BOARD_LORA_CS, HIGH);
  pinMode(BOARD_LORA_RST, OUTPUT);
  digitalWrite(BOARD_LORA_RST, HIGH);
  pinMode(BOARD_SD_CS, OUTPUT);
  digitalWrite(BOARD_SD_CS, HIGH);
  pinMode(BOARD_EPD_CS, OUTPUT);
  digitalWrite(BOARD_EPD_CS, HIGH);
}

uint8_t blackCoverageForLevel(const uint8_t level) {
  switch (level) {
    case 0:
      return 0;
    case 1:
      return 5;
    case 2:
      return 10;
    case 3:
    default:
      return 16;
  }
}
}  // namespace

HalDisplay::HalDisplay()
    : epd(GxEPD2_310_GDEQ031T10(BOARD_EPD_CS, BOARD_EPD_DC, BOARD_EPD_RST, BOARD_EPD_BUSY)) {}

HalDisplay::~HalDisplay() {
  free(frameBuffer);
  free(lastDisplayedBuffer);
  free(grayscaleLsbBuffer);
  free(grayscaleMsbBuffer);
  free(ditherBuffer);
}

void HalDisplay::ensureBuffers() {
  if (frameBuffer != nullptr) {
    return;
  }

  frameBuffer = static_cast<uint8_t*>(malloc(BUFFER_SIZE));
  lastDisplayedBuffer = static_cast<uint8_t*>(malloc(BUFFER_SIZE));
  grayscaleLsbBuffer = static_cast<uint8_t*>(malloc(BUFFER_SIZE));
  grayscaleMsbBuffer = static_cast<uint8_t*>(malloc(BUFFER_SIZE));
  ditherBuffer = static_cast<uint8_t*>(malloc(BUFFER_SIZE));

  if (!frameBuffer || !lastDisplayedBuffer || !grayscaleLsbBuffer || !grayscaleMsbBuffer || !ditherBuffer) {
    LOG_ERR("DISP", "Framebuffer allocation failed");
    assert(false);
  }

  memset(frameBuffer, 0xFF, BUFFER_SIZE);
  memset(lastDisplayedBuffer, 0xFF, BUFFER_SIZE);
  memset(grayscaleLsbBuffer, 0x00, BUFFER_SIZE);
  memset(grayscaleMsbBuffer, 0x00, BUFFER_SIZE);
  memset(ditherBuffer, 0xFF, BUFFER_SIZE);
}

void HalDisplay::begin() {
  ensureBuffers();
  deselectSharedSpiDevices();
  epd.init(115200, true, 2, false);
  epd.setRotation(0);
  flushBuffer(frameBuffer, FULL_REFRESH, true);
  initialized = true;
}

void HalDisplay::clearScreen(const uint8_t color) const {
  if (frameBuffer == nullptr) {
    return;
  }
  memset(frameBuffer, color, BUFFER_SIZE);
}

bool HalDisplay::getBufferBit(const uint8_t* buffer, const uint16_t x, const uint16_t y) {
  const uint32_t byteIndex = static_cast<uint32_t>(y) * DISPLAY_WIDTH_BYTES + (x / 8);
  const uint8_t bitMask = static_cast<uint8_t>(0x80 >> (x & 0x07));
  return (buffer[byteIndex] & bitMask) != 0;
}

void HalDisplay::setBufferPixel(uint8_t* buffer, const uint16_t x, const uint16_t y, const bool black) {
  const uint32_t byteIndex = static_cast<uint32_t>(y) * DISPLAY_WIDTH_BYTES + (x / 8);
  const uint8_t bitMask = static_cast<uint8_t>(0x80 >> (x & 0x07));
  if (black) {
    buffer[byteIndex] &= static_cast<uint8_t>(~bitMask);
  } else {
    buffer[byteIndex] |= bitMask;
  }
}

void HalDisplay::drawImage(const uint8_t* imageData, const uint16_t x, const uint16_t y, const uint16_t w,
                           const uint16_t h, const bool fromProgmem) const {
  if (!frameBuffer || !imageData) {
    return;
  }

  const uint16_t rowBytes = (w + 7) / 8;
  for (uint16_t iy = 0; iy < h; iy++) {
    if (y + iy >= DISPLAY_HEIGHT) {
      break;
    }
    for (uint16_t ix = 0; ix < w; ix++) {
      if (x + ix >= DISPLAY_WIDTH) {
        break;
      }
      const uint32_t index = static_cast<uint32_t>(iy) * rowBytes + (ix / 8);
      uint8_t byte = fromProgmem ? pgm_read_byte(&imageData[index]) : imageData[index];
      const bool black = (byte & (0x80 >> (ix & 0x07))) == 0;
      setBufferPixel(frameBuffer, x + ix, y + iy, black);
    }
  }
}

void HalDisplay::drawImageTransparent(const uint8_t* imageData, const uint16_t x, const uint16_t y, const uint16_t w,
                                      const uint16_t h, const bool fromProgmem) const {
  if (!frameBuffer || !imageData) {
    return;
  }

  const uint16_t rowBytes = (w + 7) / 8;
  for (uint16_t iy = 0; iy < h; iy++) {
    if (y + iy >= DISPLAY_HEIGHT) {
      break;
    }
    for (uint16_t ix = 0; ix < w; ix++) {
      if (x + ix >= DISPLAY_WIDTH) {
        break;
      }
      const uint32_t index = static_cast<uint32_t>(iy) * rowBytes + (ix / 8);
      uint8_t byte = fromProgmem ? pgm_read_byte(&imageData[index]) : imageData[index];
      if ((byte & (0x80 >> (ix & 0x07))) == 0) {
        setBufferPixel(frameBuffer, x + ix, y + iy, true);
      }
    }
  }
}

void HalDisplay::flushBuffer(const uint8_t* buffer, RefreshMode mode, const bool turnOffScreen) {
  if (!buffer) {
    return;
  }

  deselectSharedSpiDevices();

  if (requireFullRefresh || mode == FULL_REFRESH) {
    epd.setFullWindow();
    epd.writeImage(buffer, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, false, false);
    epd.refresh(false);
    requireFullRefresh = false;
  } else {
    epd.setPartialWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    epd.writeImage(buffer, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, false, false);
    epd.refresh(true);
  }

  if (turnOffScreen) {
    epd.powerOff();
  }
}

void HalDisplay::displayBuffer(const RefreshMode mode, const bool turnOffScreen) {
  flushBuffer(frameBuffer, mode, turnOffScreen);
  memcpy(lastDisplayedBuffer, frameBuffer, BUFFER_SIZE);
}

void HalDisplay::refreshDisplay(const RefreshMode mode, const bool turnOffScreen) {
  flushBuffer(frameBuffer, mode, turnOffScreen);
  memcpy(lastDisplayedBuffer, frameBuffer, BUFFER_SIZE);
}

void HalDisplay::requestFullRefreshNext() { requireFullRefresh = true; }

void HalDisplay::deepSleep() {
  requireFullRefresh = true;
  epd.hibernate();
}

uint8_t* HalDisplay::getFrameBuffer() const { return frameBuffer; }

void HalDisplay::copyGrayscaleBuffers(const uint8_t* lsbBuffer, const uint8_t* msbBuffer) {
  copyGrayscaleLsbBuffers(lsbBuffer);
  copyGrayscaleMsbBuffers(msbBuffer);
}

void HalDisplay::copyGrayscaleLsbBuffers(const uint8_t* lsbBuffer) {
  if (!grayscaleLsbBuffer) {
    ensureBuffers();
  }
  if (lsbBuffer) {
    memcpy(grayscaleLsbBuffer, lsbBuffer, BUFFER_SIZE);
  } else {
    memset(grayscaleLsbBuffer, 0x00, BUFFER_SIZE);
  }
}

void HalDisplay::copyGrayscaleMsbBuffers(const uint8_t* msbBuffer) {
  if (!grayscaleMsbBuffer) {
    ensureBuffers();
  }
  if (msbBuffer) {
    memcpy(grayscaleMsbBuffer, msbBuffer, BUFFER_SIZE);
  } else {
    memset(grayscaleMsbBuffer, 0x00, BUFFER_SIZE);
  }
}

void HalDisplay::cleanupGrayscaleBuffers(const uint8_t* bwBuffer) {
  if (bwBuffer) {
    memcpy(lastDisplayedBuffer, bwBuffer, BUFFER_SIZE);
  }
  memset(grayscaleLsbBuffer, 0x00, BUFFER_SIZE);
  memset(grayscaleMsbBuffer, 0x00, BUFFER_SIZE);
}

void HalDisplay::displayGrayBuffer(const bool turnOffScreen) {
  if (!ditherBuffer || !grayscaleLsbBuffer || !grayscaleMsbBuffer) {
    ensureBuffers();
  }

  memset(ditherBuffer, 0xFF, BUFFER_SIZE);
  const uint8_t* baseBuffer = lastDisplayedBuffer ? lastDisplayedBuffer : frameBuffer;

  for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++) {
    for (uint16_t x = 0; x < DISPLAY_WIDTH; x++) {
      const bool lsbMarked = getBufferBit(grayscaleLsbBuffer, x, y);
      const bool msbMarked = getBufferBit(grayscaleMsbBuffer, x, y);
      const bool baseBlack = !getBufferBit(baseBuffer, x, y);

      uint8_t grayLevel = 0;
      if (msbMarked && lsbMarked) {
        grayLevel = 2;
      } else if (msbMarked || lsbMarked) {
        grayLevel = 1;
      } else if (baseBlack) {
        grayLevel = 3;
      }

      const uint8_t coverage = blackCoverageForLevel(grayLevel);
      const bool black = coverage > kBayer4x4[y & 0x03][x & 0x03];
      setBufferPixel(ditherBuffer, x, y, black);
    }
  }

  flushBuffer(ditherBuffer, FAST_REFRESH, turnOffScreen);
}

uint16_t HalDisplay::getDisplayWidth() const { return DISPLAY_WIDTH; }

uint16_t HalDisplay::getDisplayHeight() const { return DISPLAY_HEIGHT; }

uint16_t HalDisplay::getDisplayWidthBytes() const { return DISPLAY_WIDTH_BYTES; }

uint32_t HalDisplay::getBufferSize() const { return BUFFER_SIZE; }
