#include "ScreenshotUtil.h"

#include <Arduino.h>
#include <BitmapHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>

#include <cstring>
#include <memory>
#include <string>

#include "Bitmap.h"  // Required for BmpHeader struct definition

void ScreenshotUtil::takeScreenshot(GfxRenderer& renderer) {
  const uint8_t* fb = renderer.getFrameBuffer();
  if (fb) {
    String filename_str = "/screenshots/screenshot-" + String(millis()) + ".bmp";
    if (ScreenshotUtil::saveFramebufferAsBmp(filename_str.c_str(), renderer)) {
      LOG_DBG("SCR", "Screenshot saved to %s", filename_str.c_str());
    } else {
      LOG_ERR("SCR", "Failed to save screenshot");
    }
  } else {
    LOG_ERR("SCR", "Framebuffer not available");
  }

  // Display a border around the screen to indicate a screenshot was taken
  if (renderer.storeBwBuffer()) {
    renderer.drawRect(6, 6, renderer.getScreenWidth() - 12, renderer.getScreenHeight() - 12, 2, true);
    renderer.displayBuffer();
    delay(1000);
    renderer.restoreBwBuffer();
    renderer.displayBuffer(HalDisplay::RefreshMode::HALF_REFRESH);
  }
}

namespace {
void rotateCoordinates(const GfxRenderer::Orientation orientation, const int x, const int y, int& phyX, int& phyY,
                       const int panelWidth, const int panelHeight) {
  switch (orientation) {
    case GfxRenderer::Portrait:
      phyX = x;
      phyY = y;
      break;
    case GfxRenderer::LandscapeClockwise:
      phyX = y;
      phyY = panelHeight - 1 - x;
      break;
    case GfxRenderer::PortraitInverted:
      phyX = panelWidth - 1 - x;
      phyY = panelHeight - 1 - y;
      break;
    case GfxRenderer::LandscapeCounterClockwise:
      phyX = panelWidth - 1 - y;
      phyY = x;
      break;
  }
}
}  // namespace

bool ScreenshotUtil::saveFramebufferAsBmp(const char* filename, const GfxRenderer& renderer) {
  const uint8_t* framebuffer = renderer.getFrameBuffer();
  if (!framebuffer) {
    return false;
  }
  const int logicalWidth = renderer.getScreenWidth();
  const int logicalHeight = renderer.getScreenHeight();
  const int physicalWidth = renderer.getDisplayWidth();
  const int physicalHeight = renderer.getDisplayHeight();
  const int physicalStride = renderer.getDisplayWidthBytes();

  std::string path(filename);
  size_t last_slash = path.find_last_of('/');
  if (last_slash != std::string::npos) {
    std::string dir = path.substr(0, last_slash);
    if (!Storage.exists(dir.c_str())) {
      if (!Storage.mkdir(dir.c_str())) {
        return false;
      }
    }
  }

  FsFile file;
  if (!Storage.openFileForWrite("SCR", filename, file)) {
    LOG_ERR("SCR", "Failed to save screenshot");
    return false;
  }

  BmpHeader header;
  createBmpHeader(&header, logicalWidth, logicalHeight, BmpRowOrder::BottomUp);

  bool write_error = false;
  if (file.write(reinterpret_cast<uint8_t*>(&header), sizeof(header)) != sizeof(header)) {
    write_error = true;
  }

  if (write_error) {
    // Explicitly close() file before calling Storage.remove()
    file.close();
    Storage.remove(filename);
    return false;
  }

  const uint32_t rowSizePadded = (logicalWidth + 31) / 32 * 4;
  auto rowBuffer = std::make_unique<uint8_t[]>(rowSizePadded);
  if (!rowBuffer) {
    file.close();
    Storage.remove(filename);
    return false;
  }

  for (int outY = 0; outY < logicalHeight; ++outY) {
    std::memset(rowBuffer.get(), 0, rowSizePadded);
    const int logicalY = logicalHeight - 1 - outY;

    for (int logicalX = 0; logicalX < logicalWidth; ++logicalX) {
      int phyX = 0;
      int phyY = 0;
      rotateCoordinates(renderer.getOrientation(), logicalX, logicalY, phyX, phyY, physicalWidth, physicalHeight);
      const int fbIndex = phyY * physicalStride + (phyX / 8);
      const uint8_t pixel = static_cast<uint8_t>((framebuffer[fbIndex] >> (7 - (phyX % 8))) & 0x01);
      rowBuffer[logicalX / 8] |= static_cast<uint8_t>(pixel << (7 - (logicalX % 8)));
    }

    if (file.write(rowBuffer.get(), rowSizePadded) != rowSizePadded) {
      write_error = true;
      break;
    }
  }

  // Explicitly close() file before calling Storage.remove()
  file.close();

  if (write_error) {
    Storage.remove(filename);
    return false;
  }

  return true;
}
