#include "LyraTheme.h"

#include <GfxRenderer.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "components/icons/book.h"
#include "components/icons/book24.h"
#include "components/icons/cover.h"
#include "components/icons/file24.h"
#include "components/icons/folder.h"
#include "components/icons/folder24.h"
#include "components/icons/hotspot.h"
#include "components/icons/image24.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/settings2.h"
#include "components/icons/text24.h"
#include "components/icons/transfer.h"
#include "components/icons/wifi.h"
#include "fontIds.h"

// Internal constants
namespace {
constexpr int hPaddingInSelection = 8;
constexpr int cornerRadius = 6;
constexpr int popupMarginX = 16;
constexpr int popupMarginY = 12;
constexpr int maxListValueWidth = 200;
constexpr int mainMenuIconSize = 32;
constexpr int listIconSize = 24;
constexpr int mainMenuColumns = 2;
constexpr int lyraHeaderBatteryYOffset = 2;
constexpr int lyraHeaderTitleYOffset = -12;
int coverWidth = 0;

void drawLyraBatteryIcon(const GfxRenderer& renderer, int x, int y, int battWidth, int rectHeight,
                         uint16_t percentage) {
  BaseTheme::drawBatteryOutline(renderer, x, y, battWidth, rectHeight);

  const bool charging = gpio.isUsbConnected();

  if (charging) {
    // Draw solid fill when charging so lightning bolt is visible
    renderer.fillRect(x + 2, y + 2, battWidth - 5, rectHeight - 4);
    BaseTheme::drawBatteryLightningBolt(renderer, x + 4, y + 2);
  } else {
    // Draw bars when not charging
    if (percentage > 10) {
      renderer.fillRect(x + 2, y + 2, 3, rectHeight - 4);
    }
    if (percentage > 40) {
      renderer.fillRect(x + 6, y + 2, 3, rectHeight - 4);
    }
    if (percentage > 70) {
      renderer.fillRect(x + 10, y + 2, 3, rectHeight - 4);
    }
  }
}

const uint8_t* iconForName(UIIcon icon, int size) {
  if (size == 24) {
    switch (icon) {
      case UIIcon::Folder:
        return Folder24Icon;
      case UIIcon::Text:
        return Text24Icon;
      case UIIcon::Image:
        return Image24Icon;
      case UIIcon::Book:
        return Book24Icon;
      case UIIcon::File:
        return File24Icon;
      default:
        return nullptr;
    }
  } else if (size == 32) {
    switch (icon) {
      case UIIcon::Folder:
        return FolderIcon;
      case UIIcon::Book:
        return BookIcon;
      case UIIcon::Recent:
        return RecentIcon;
      case UIIcon::Settings:
        return Settings2Icon;
      case UIIcon::Transfer:
        return TransferIcon;
      case UIIcon::Library:
        return LibraryIcon;
      case UIIcon::Wifi:
        return WifiIcon;
      case UIIcon::Hotspot:
        return HotspotIcon;
      default:
        return nullptr;
    }
  }
  return nullptr;
}
}  // namespace

void LyraTheme::drawBatteryLeft(const GfxRenderer& renderer, Rect rect, const bool showPercentage) const {
  // Left aligned: icon on left, percentage on right (reader mode)
  const uint16_t percentage = powerManager.getBatteryPercentage();

  if (showPercentage) {
    const auto percentageText = std::to_string(percentage) + "%";
    renderer.drawText(SMALL_FONT_ID, rect.x + BaseTheme::batteryPercentSpacing + LyraMetrics::values.batteryWidth,
                      rect.y, percentageText.c_str());
  }

  drawLyraBatteryIcon(renderer, rect.x, rect.y + 6, LyraMetrics::values.batteryWidth, rect.height, percentage);
}

void LyraTheme::drawBatteryRight(const GfxRenderer& renderer, Rect rect, const bool showPercentage) const {
  // Right aligned: percentage on left, icon on right (UI headers)
  const uint16_t percentage = powerManager.getBatteryPercentage();

  if (showPercentage) {
    const auto percentageText = std::to_string(percentage) + "%";
    const int textWidth = renderer.getTextWidth(SMALL_FONT_ID, percentageText.c_str());
    // Clear the area where we're going to draw the text to prevent ghosting
    const auto textHeight = renderer.getTextHeight(SMALL_FONT_ID);
    renderer.fillRect(rect.x - textWidth - BaseTheme::batteryPercentSpacing, rect.y, textWidth, textHeight, false);
    // Draw text to the left of the icon
    renderer.drawText(SMALL_FONT_ID, rect.x - textWidth - BaseTheme::batteryPercentSpacing, rect.y,
                      percentageText.c_str());
  }

  drawLyraBatteryIcon(renderer, rect.x, rect.y + 6, LyraMetrics::values.batteryWidth, rect.height, percentage);
}

void LyraTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle) const {
  renderer.fillRect(rect.x, rect.y, rect.width, rect.height, false);

  const bool showBatteryPercentage =
      SETTINGS.hideBatteryPercentage != CrossPointSettings::HIDE_BATTERY_PERCENTAGE::HIDE_ALWAYS;
  const auto batteryPercent = std::to_string(powerManager.getBatteryPercentage()) + "%";
  const int batteryTextWidth =
      showBatteryPercentage ? renderer.getTextWidth(SMALL_FONT_ID, batteryPercent.c_str()) + BaseTheme::batteryPercentSpacing
                            : 0;
  // Position icon at right edge, drawBatteryRight will place text to the left
  const int batteryX = rect.x + rect.width - 12 - LyraMetrics::values.batteryWidth;
  drawBatteryRight(renderer,
                   Rect{batteryX, rect.y + lyraHeaderBatteryYOffset, LyraMetrics::values.batteryWidth,
                        LyraMetrics::values.batteryHeight},
                   showBatteryPercentage);

  const int textLeft = rect.x + LyraMetrics::values.contentSidePadding;
  const int textRight = batteryX - batteryTextWidth - LyraMetrics::values.contentSidePadding;
  const int availableSpace = std::max(0, textRight - textLeft);
  const int underlineY = rect.y + rect.height - 3;
  const int titleHeight = renderer.getLineHeight(UI_12_FONT_ID);
  const int subtitleHeight = renderer.getLineHeight(SMALL_FONT_ID);
  const int textAreaTop = rect.y + LyraMetrics::values.batteryBarHeight;
  const int textAreaHeight = std::max(0, underlineY - textAreaTop - 1);
  const int titleY = textAreaTop + std::max(0, (textAreaHeight - titleHeight) / 2) + lyraHeaderTitleYOffset;
  const int subtitleY = textAreaTop + std::max(0, (textAreaHeight - subtitleHeight) / 2) + lyraHeaderTitleYOffset;

  if (title) {
    int maxTitleWidth = availableSpace;
    int maxSubtitleWidth = 0;
    if (subtitle != nullptr) {
      const int gap = hPaddingInSelection;
      maxSubtitleWidth = std::min(renderer.getTextWidth(SMALL_FONT_ID, subtitle, EpdFontFamily::REGULAR),
                                  std::max(availableSpace / 2, 0));
      maxTitleWidth = std::max(0, availableSpace - maxSubtitleWidth - gap);
      if (maxTitleWidth < availableSpace / 3) {
        maxSubtitleWidth = std::max(0, availableSpace / 3);
        maxTitleWidth = std::max(0, availableSpace - maxSubtitleWidth - gap);
      }
    }

    auto truncatedTitle = renderer.truncatedText(UI_12_FONT_ID, title, maxTitleWidth, EpdFontFamily::BOLD);
    renderer.drawText(UI_12_FONT_ID, textLeft, titleY, truncatedTitle.c_str(), true, EpdFontFamily::BOLD);
    renderer.drawLine(rect.x, rect.y + rect.height - 3, rect.x + rect.width - 1, rect.y + rect.height - 3, 3, true);
    if (subtitle && maxSubtitleWidth > 0) {
      auto truncatedSubtitle =
          renderer.truncatedText(SMALL_FONT_ID, subtitle, maxSubtitleWidth, EpdFontFamily::REGULAR);
      int truncatedSubtitleWidth = renderer.getTextWidth(SMALL_FONT_ID, truncatedSubtitle.c_str());
      renderer.drawText(SMALL_FONT_ID, textRight - truncatedSubtitleWidth, subtitleY, truncatedSubtitle.c_str(), true);
    }
  }
}

void LyraTheme::drawSubHeader(const GfxRenderer& renderer, Rect rect, const char* label, const char* rightLabel) const {
  int currentX = rect.x + LyraMetrics::values.contentSidePadding;
  int rightSpace = LyraMetrics::values.contentSidePadding;
  const int smallTextY = rect.y + std::max(0, (rect.height - renderer.getLineHeight(SMALL_FONT_ID)) / 2);
  const int textY = rect.y + std::max(0, (rect.height - renderer.getLineHeight(UI_10_FONT_ID)) / 2);
  if (rightLabel) {
    auto truncatedRightLabel =
        renderer.truncatedText(SMALL_FONT_ID, rightLabel, maxListValueWidth, EpdFontFamily::REGULAR);
    int rightLabelWidth = renderer.getTextWidth(SMALL_FONT_ID, truncatedRightLabel.c_str());
    renderer.drawText(SMALL_FONT_ID, rect.x + rect.width - LyraMetrics::values.contentSidePadding - rightLabelWidth,
                      smallTextY, truncatedRightLabel.c_str());
    rightSpace += rightLabelWidth + hPaddingInSelection;
  }

  auto truncatedLabel = renderer.truncatedText(
      UI_10_FONT_ID, label, rect.width - LyraMetrics::values.contentSidePadding - rightSpace, EpdFontFamily::REGULAR);
  renderer.drawText(UI_10_FONT_ID, currentX, textY, truncatedLabel.c_str(), true, EpdFontFamily::REGULAR);

  renderer.drawLine(rect.x, rect.y + rect.height - 1, rect.x + rect.width - 1, rect.y + rect.height - 1, true);
}

void LyraTheme::drawTabBar(const GfxRenderer& renderer, Rect rect, const std::vector<TabInfo>& tabs,
                           bool selected) const {
  if (tabs.empty()) {
    renderer.drawLine(rect.x, rect.y + rect.height - 1, rect.x + rect.width - 1, rect.y + rect.height - 1, true);
    return;
  }

  if (selected) {
    renderer.fillRectDither(rect.x, rect.y, rect.width, rect.height, Color::LightGray);
  }

  const int tabCount = static_cast<int>(tabs.size());
  const int edgePadding = std::max(4, LyraMetrics::values.contentSidePadding - 4);
  const int slotSpacing = std::max(2, LyraMetrics::values.tabSpacing / 2);
  const int availableWidth = rect.width - edgePadding * 2 - slotSpacing * (tabCount - 1);
  const int tabWidth = std::max(18, availableWidth / tabCount);
  const int innerPadding = 3;

  int fontId = UI_10_FONT_ID;
  for (const auto& tab : tabs) {
    if (renderer.getTextWidth(fontId, tab.label, EpdFontFamily::REGULAR) > tabWidth - innerPadding * 2) {
      fontId = SMALL_FONT_ID;
      break;
    }
  }

  const int textY = rect.y + std::max(0, (rect.height - renderer.getLineHeight(fontId)) / 2);
  int currentX = rect.x + edgePadding;

  for (int index = 0; index < tabCount; ++index) {
    const auto& tab = tabs[index];
    const int slotX = currentX;
    const int slotW = (index == tabCount - 1) ? (rect.x + rect.width - edgePadding - slotX) : tabWidth;
    auto label = renderer.truncatedText(fontId, tab.label, std::max(0, slotW - innerPadding * 2));
    const int textWidth = renderer.getTextWidth(fontId, label.c_str(), EpdFontFamily::REGULAR);

    if (tab.selected) {
      if (selected) {
        renderer.fillRoundedRect(slotX, rect.y + 1, slotW, rect.height - 4, cornerRadius, Color::Black);
      } else {
        renderer.fillRectDither(slotX, rect.y, slotW, rect.height - 3, Color::LightGray);
        renderer.drawLine(slotX, rect.y + rect.height - 3, slotX + slotW, rect.y + rect.height - 3, 2, true);
      }
    }

    renderer.drawText(fontId, slotX + std::max(0, (slotW - textWidth) / 2), textY, label.c_str(),
                      !(tab.selected && selected), EpdFontFamily::REGULAR);

    currentX += slotW + slotSpacing;
  }

  renderer.drawLine(rect.x, rect.y + rect.height - 1, rect.x + rect.width - 1, rect.y + rect.height - 1, true);
}

void LyraTheme::drawList(const GfxRenderer& renderer, Rect rect, int itemCount, int selectedIndex,
                         const std::function<std::string(int index)>& rowTitle,
                         const std::function<std::string(int index)>& rowSubtitle,
                         const std::function<UIIcon(int index)>& rowIcon,
                         const std::function<std::string(int index)>& rowValue, bool highlightValue) const {
  int rowHeight =
      (rowSubtitle != nullptr) ? LyraMetrics::values.listWithSubtitleRowHeight : LyraMetrics::values.listRowHeight;
  int pageItems = std::max(1, rect.height / rowHeight);

  const int totalPages = (itemCount + pageItems - 1) / pageItems;
  if (totalPages > 1) {
    const int scrollAreaHeight = rect.height;

    // Draw scroll bar
    const int scrollBarHeight = (scrollAreaHeight * pageItems) / itemCount;
    const int currentPage = selectedIndex / pageItems;
    const int scrollBarY = rect.y + ((scrollAreaHeight - scrollBarHeight) * currentPage) / (totalPages - 1);
    const int scrollBarX = rect.x + rect.width - LyraMetrics::values.scrollBarRightOffset;
    renderer.drawLine(scrollBarX, rect.y, scrollBarX, rect.y + scrollAreaHeight, true);
    renderer.fillRect(scrollBarX - LyraMetrics::values.scrollBarWidth, scrollBarY, LyraMetrics::values.scrollBarWidth,
                      scrollBarHeight, true);
  }

  // Draw selection
  int contentWidth =
      rect.width -
      (totalPages > 1 ? (LyraMetrics::values.scrollBarWidth + LyraMetrics::values.scrollBarRightOffset) : 1);
  if (selectedIndex >= 0) {
    renderer.fillRoundedRect(LyraMetrics::values.contentSidePadding, rect.y + selectedIndex % pageItems * rowHeight,
                             contentWidth - LyraMetrics::values.contentSidePadding * 2, rowHeight, cornerRadius,
                             Color::LightGray);
  }

  int textX = rect.x + LyraMetrics::values.contentSidePadding + hPaddingInSelection;
  int textWidth = contentWidth - LyraMetrics::values.contentSidePadding * 2 - hPaddingInSelection * 2;
  int iconSize = 0;
  if (rowIcon != nullptr) {
    iconSize = (rowSubtitle != nullptr) ? mainMenuIconSize : listIconSize;
    textX += iconSize + hPaddingInSelection;
    textWidth -= iconSize + hPaddingInSelection;
  }

  // Draw all items
  const auto pageStartIndex = selectedIndex / pageItems * pageItems;
  for (int i = pageStartIndex; i < itemCount && i < pageStartIndex + pageItems; i++) {
    const int itemY = rect.y + (i % pageItems) * rowHeight;
    int rowTextWidth = textWidth;
    const int titleFont = (rowSubtitle != nullptr) ? UI_10_FONT_ID : SMALL_FONT_ID;
    const int valueFont = SMALL_FONT_ID;
    const int titleLineHeight = renderer.getLineHeight(titleFont);
    const int subtitleLineHeight = (rowSubtitle != nullptr) ? renderer.getLineHeight(SMALL_FONT_ID) : 0;
    const int textGap = (rowSubtitle != nullptr) ? 2 : 0;
    const int textBlockHeight = titleLineHeight + (rowSubtitle != nullptr ? subtitleLineHeight + textGap : 0);
    const int textTop = itemY + std::max(0, (rowHeight - textBlockHeight) / 2);
    const int titleY = textTop;
    const int subtitleY = textTop + titleLineHeight + textGap;
    const int valueY = itemY + std::max(0, (rowHeight - renderer.getLineHeight(valueFont)) / 2);

    // Draw name
    int valueWidth = 0;
    std::string valueText = "";
    if (rowValue != nullptr) {
      valueText = rowValue(i);
      valueText = renderer.truncatedText(valueFont, valueText.c_str(), maxListValueWidth);
      valueWidth = renderer.getTextWidth(valueFont, valueText.c_str()) + hPaddingInSelection;
      rowTextWidth -= valueWidth;
    }

    auto itemName = rowTitle(i);
    auto item = renderer.truncatedText(titleFont, itemName.c_str(), rowTextWidth);
    renderer.drawText(titleFont, textX, titleY, item.c_str(), true);

    if (rowIcon != nullptr) {
      UIIcon icon = rowIcon(i);
      const uint8_t* iconBitmap = iconForName(icon, iconSize);
      if (iconBitmap != nullptr) {
        const int iconY = itemY + std::max(0, (rowHeight - iconSize) / 2);
        renderer.drawIcon(iconBitmap, rect.x + LyraMetrics::values.contentSidePadding + hPaddingInSelection,
                          iconY, iconSize, iconSize);
      }
    }

    if (rowSubtitle != nullptr) {
      // Draw subtitle
      std::string subtitleText = rowSubtitle(i);
      auto subtitle = renderer.truncatedText(SMALL_FONT_ID, subtitleText.c_str(), rowTextWidth);
      renderer.drawText(SMALL_FONT_ID, textX, subtitleY, subtitle.c_str(), true);
    }

    // Draw value
    if (!valueText.empty()) {
      if (i == selectedIndex && highlightValue) {
        renderer.fillRoundedRect(
            contentWidth - LyraMetrics::values.contentSidePadding - hPaddingInSelection - valueWidth, itemY,
            valueWidth + hPaddingInSelection, rowHeight, cornerRadius, Color::Black);
      }

      renderer.drawText(valueFont, rect.x + contentWidth - LyraMetrics::values.contentSidePadding - valueWidth, valueY,
                        valueText.c_str(), !(i == selectedIndex && highlightValue));
    }
  }
}

void LyraTheme::drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                                const char* btn4) const {
  if (!UITheme::showsBottomButtonHints() || LyraMetrics::values.buttonHintsHeight <= 0) {
    return;
  }

  const GfxRenderer::Orientation orig_orientation = renderer.getOrientation();
  renderer.setOrientation(GfxRenderer::Orientation::Portrait);

  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const int margin = LyraMetrics::values.contentSidePadding;
  const int gap = 4;
  const int buttonHeight = LyraMetrics::values.buttonHintsHeight;
  const int smallButtonHeight = std::max(12, buttonHeight - 8);
  const int buttonY = pageHeight - buttonHeight - 4;
  const int smallButtonY = pageHeight - smallButtonHeight - 4;
  const int buttonWidth = std::max(28, (pageWidth - margin * 2 - gap * 3) / 4);
  const int textYOffset = std::max(2, (buttonHeight - renderer.getLineHeight(SMALL_FONT_ID)) / 2);
  const char* labels[] = {btn1, btn2, btn3, btn4};

  for (int i = 0; i < 4; i++) {
    const int x = margin + i * (buttonWidth + gap);
    if (labels[i] != nullptr && labels[i][0] != '\0') {
      // Draw the filled background and border for a FULL-sized button
      renderer.fillRoundedRect(x, buttonY, buttonWidth, buttonHeight, cornerRadius, Color::White);
      renderer.drawRoundedRect(x, buttonY, buttonWidth, buttonHeight, 1, cornerRadius, true, true, false, false,
                               true);
      const int textWidth = renderer.getTextWidth(SMALL_FONT_ID, labels[i]);
      const int textX = x + (buttonWidth - 1 - textWidth) / 2;
      renderer.drawText(SMALL_FONT_ID, textX, buttonY + textYOffset, labels[i]);
    } else {
      // Draw the filled background and border for a SMALL-sized button
      renderer.fillRoundedRect(x, smallButtonY, buttonWidth, smallButtonHeight, cornerRadius, Color::White);
      renderer.drawRoundedRect(x, smallButtonY, buttonWidth, smallButtonHeight, 1, cornerRadius, true, true, false,
                               false, true);
    }
  }

  renderer.setOrientation(orig_orientation);
}

void LyraTheme::drawSideButtonHints(const GfxRenderer& renderer, const char* topBtn, const char* bottomBtn) const {
  const int screenWidth = renderer.getScreenWidth();
  const int screenHeight = renderer.getScreenHeight();
  const int buttonWidth = LyraMetrics::values.sideButtonHintsWidth;
  const int buttonHeight = std::min(60, std::max(44, screenHeight / 5));
  const int buttonY = (screenHeight - buttonHeight) / 2;

  if (topBtn != nullptr && topBtn[0] != '\0') {
    renderer.drawRoundedRect(0, buttonY, buttonWidth, buttonHeight, 1, cornerRadius, false, true, false, true, true);
    const int textWidth = renderer.getTextWidth(SMALL_FONT_ID, topBtn);
    renderer.drawTextRotated90CW(SMALL_FONT_ID, 0, buttonY + (buttonHeight + textWidth) / 2, topBtn);
  }

  if (bottomBtn != nullptr && bottomBtn[0] != '\0') {
    const int rightX = screenWidth - buttonWidth;
    renderer.drawRoundedRect(rightX, buttonY, buttonWidth, buttonHeight, 1, cornerRadius, true, false, true, false,
                             true);
    const int textWidth = renderer.getTextWidth(SMALL_FONT_ID, bottomBtn);
    renderer.drawTextRotated90CW(SMALL_FONT_ID, rightX, buttonY + (buttonHeight + textWidth) / 2, bottomBtn);
  }
}

void LyraTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                    const int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                    bool& bufferRestored, std::function<bool()> storeCoverBuffer) const {
  constexpr int kHomeTitleFontId = UI_10_FONT_ID;
  constexpr int kHomeAuthorFontId = SMALL_FONT_ID;
  constexpr int kHomeTitleMaxLines = 3;
  constexpr int kHomeAuthorGap = 2;
  constexpr int kHomeSelectionTopOffset = 6;

  const int tileWidth = rect.width - 2 * LyraMetrics::values.contentSidePadding;
  const int tileHeight = rect.height;
  const int tileY = rect.y;
  const int coverHeight = LyraMetrics::values.homeCoverHeight;
  const int coverVerticalInset = std::max(0, (tileHeight - coverHeight) / 2);
  const bool hasContinueReading = !recentBooks.empty();
  if (coverWidth == 0) {
    coverWidth = coverHeight * 0.6;
  }

  // Draw book card regardless, fill with message based on `hasContinueReading`
  // Draw cover image as background if available (inside the box)
  // Only load from SD on first render, then use stored buffer
  if (hasContinueReading) {
    RecentBook book = recentBooks[0];
    if (!coverRendered) {
      std::string coverPath = book.coverBmpPath;
      bool hasCover = true;
      int tileX = LyraMetrics::values.contentSidePadding;
      const int coverX = tileX + hPaddingInSelection;
      const int coverY = tileY + coverVerticalInset;
      if (coverPath.empty()) {
        hasCover = false;
      } else {
        const std::string coverBmpPath = UITheme::getCoverThumbPath(coverPath, coverHeight);

        // First time: load cover from SD and render
        FsFile file;
        if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
          Bitmap bitmap(file);
          if (bitmap.parseHeaders() == BmpReaderError::Ok) {
            const int bitmapWidth = bitmap.getWidth();
            const int bitmapHeight = bitmap.getHeight();
            float cropX = 0.0f;
            float cropY = 0.0f;

            if (bitmapWidth > 0 && bitmapHeight > 0 && coverWidth > 0 && coverHeight > 0) {
              const float bitmapAspect = static_cast<float>(bitmapWidth) / static_cast<float>(bitmapHeight);
              const float targetAspect = static_cast<float>(coverWidth) / static_cast<float>(coverHeight);

              if (bitmapAspect > targetAspect) {
                cropX = 1.0f - (targetAspect / bitmapAspect);
              } else if (bitmapAspect < targetAspect) {
                cropY = 1.0f - (bitmapAspect / targetAspect);
              }

              cropX = std::clamp(cropX, 0.0f, 0.95f);
              cropY = std::clamp(cropY, 0.0f, 0.95f);
            }

            renderer.drawBitmap(bitmap, coverX, coverY, coverWidth, coverHeight, cropX, cropY);
          } else {
            hasCover = false;
          }
          file.close();
        }
      }

      // Draw either way
      renderer.drawRect(coverX, coverY, coverWidth, coverHeight, true);

      if (!hasCover) {
        // Render empty cover
        renderer.fillRect(coverX, coverY + (coverHeight / 3), coverWidth, 2 * coverHeight / 3, true);
        renderer.drawIcon(CoverIcon, coverX + 24, coverY + 24, 32, 32);
      }

      coverBufferStored = storeCoverBuffer();
      coverRendered = coverBufferStored;  // Only consider it rendered if we successfully stored the buffer
    }

    bool bookSelected = (selectorIndex == 0);

    int tileX = LyraMetrics::values.contentSidePadding;
    const int coverX = tileX + hPaddingInSelection;
    const int coverY = tileY + coverVerticalInset;
    const int coverRight = coverX + coverWidth;
    int textWidth = tileWidth - 2 * hPaddingInSelection - LyraMetrics::values.verticalSpacing - coverWidth;

    if (bookSelected) {
      const int selectedTopOffset = std::min(coverVerticalInset, kHomeSelectionTopOffset);
      const int selectedTopY = tileY + selectedTopOffset;
      const int selectedTopHeight = std::max(0, coverVerticalInset - selectedTopOffset);

      if (selectedTopHeight > 0) {
        renderer.fillRectDither(tileX, selectedTopY, tileWidth, selectedTopHeight, Color::LightGray);
      }
      renderer.fillRectDither(tileX, coverY, hPaddingInSelection, coverHeight, Color::LightGray);
      renderer.fillRectDither(coverRight, coverY, tileWidth - (coverRight - tileX), coverHeight, Color::LightGray);
      const int bottomFillY = coverY + coverHeight;
      const int bottomFillHeight = std::max(0, tileHeight - (bottomFillY - tileY));
      if (bottomFillHeight > 0) {
        renderer.fillRectDither(tileX, bottomFillY, tileWidth, bottomFillHeight, Color::LightGray);
      }
    }

    auto titleLines =
        renderer.wrappedText(kHomeTitleFontId, book.title.c_str(), textWidth, kHomeTitleMaxLines, EpdFontFamily::BOLD);
    auto author = renderer.truncatedText(kHomeAuthorFontId, book.author.c_str(), textWidth);
    const int titleLineHeight = renderer.getLineHeight(kHomeTitleFontId);
    const int authorLineHeight = renderer.getLineHeight(kHomeAuthorFontId);
    const int titleBlockHeight = titleLineHeight * static_cast<int>(titleLines.size());
    const int authorHeight = book.author.empty() ? 0 : (authorLineHeight + kHomeAuthorGap);
    const int totalBlockHeight = titleBlockHeight + authorHeight;
    int titleY = coverY + std::max(0, (coverHeight - totalBlockHeight) / 2);
    const int textX = tileX + hPaddingInSelection + coverWidth + LyraMetrics::values.verticalSpacing;
    for (const auto& line : titleLines) {
      renderer.drawText(kHomeTitleFontId, textX, titleY, line.c_str(), true, EpdFontFamily::BOLD);
      titleY += titleLineHeight;
    }
    if (!book.author.empty()) {
      titleY += kHomeAuthorGap;
      renderer.drawText(kHomeAuthorFontId, textX, titleY, author.c_str(), true);
    }
  } else {
    drawEmptyRecents(renderer, rect);
  }
}

void LyraTheme::drawEmptyRecents(const GfxRenderer& renderer, const Rect rect) const {
  constexpr int padding = 48;
  renderer.drawText(UI_10_FONT_ID, rect.x + padding,
                    rect.y + rect.height / 2 - renderer.getLineHeight(UI_10_FONT_ID) - 2, tr(STR_NO_OPEN_BOOK), true,
                    EpdFontFamily::BOLD);
  renderer.drawText(SMALL_FONT_ID, rect.x + padding, rect.y + rect.height / 2 + 2, tr(STR_START_READING), true);
}

void LyraTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                               const std::function<std::string(int index)>& buttonLabel,
                               const std::function<UIIcon(int index)>& rowIcon) const {
  constexpr int kHomeMenuFontId = UI_10_FONT_ID;
  constexpr int kHomeMenuTextLeftPadding = 12;
  constexpr int kHomeMenuIconTextGap = 8;

  for (int i = 0; i < buttonCount; ++i) {
    int tileWidth = rect.width - LyraMetrics::values.contentSidePadding * 2;
    Rect tileRect = Rect{rect.x + LyraMetrics::values.contentSidePadding,
                         rect.y + i * (LyraMetrics::values.menuRowHeight + LyraMetrics::values.menuSpacing), tileWidth,
                         LyraMetrics::values.menuRowHeight};

    const bool selected = selectedIndex == i;

    if (selected) {
      renderer.fillRoundedRect(tileRect.x, tileRect.y, tileRect.width, tileRect.height, cornerRadius, Color::LightGray);
    }

    std::string labelStr = buttonLabel(i);
    const char* label = labelStr.c_str();
    int textX = tileRect.x + kHomeMenuTextLeftPadding;
    const int lineHeight = renderer.getLineHeight(kHomeMenuFontId);
    const int textY = tileRect.y + (LyraMetrics::values.menuRowHeight - lineHeight) / 2;

    if (rowIcon != nullptr) {
      UIIcon icon = rowIcon(i);
      const uint8_t* iconBitmap = iconForName(icon, mainMenuIconSize);
      if (iconBitmap != nullptr) {
        const int iconY = tileRect.y + (LyraMetrics::values.menuRowHeight - mainMenuIconSize) / 2;
        renderer.drawIcon(iconBitmap, textX, iconY, mainMenuIconSize, mainMenuIconSize);
        textX += mainMenuIconSize + kHomeMenuIconTextGap;
      }
    }

    renderer.drawText(kHomeMenuFontId, textX, textY, label, true);
  }
}

Rect LyraTheme::drawPopup(const GfxRenderer& renderer, const char* message) const {
  // Scale y position proportionally to screen height (16.5% from top)
  const int y = static_cast<int>(renderer.getScreenHeight() * 0.165f);
  constexpr int outline = 2;
  const int textWidth = renderer.getTextWidth(UI_12_FONT_ID, message, EpdFontFamily::REGULAR);
  const int textHeight = renderer.getLineHeight(UI_12_FONT_ID);
  const int w = textWidth + popupMarginX * 2;
  const int h = textHeight + popupMarginY * 2;
  const int x = (renderer.getScreenWidth() - w) / 2;

  renderer.fillRoundedRect(x - outline, y - outline, w + outline * 2, h + outline * 2, cornerRadius + outline,
                           Color::White);
  renderer.fillRoundedRect(x, y, w, h, cornerRadius, Color::Black);

  const int textX = x + (w - textWidth) / 2;
  const int textY = y + popupMarginY - 2;
  renderer.drawText(UI_12_FONT_ID, textX, textY, message, false, EpdFontFamily::REGULAR);
  renderer.displayBuffer();

  return Rect{x, y, w, h};
}

void LyraTheme::fillPopupProgress(const GfxRenderer& renderer, const Rect& layout, const int progress) const {
  constexpr int barHeight = 4;

  // Twice the margin in drawPopup to match text width
  const int barWidth = layout.width - popupMarginX * 2;
  const int barX = layout.x + (layout.width - barWidth) / 2;
  // Center inside the margin of drawPopup. The - 1 is added to account for the - 2 in drawPopup.
  const int barY = layout.y + layout.height - popupMarginY / 2 - barHeight / 2 - 1;

  int fillWidth = barWidth * progress / 100;

  renderer.fillRect(barX, barY, fillWidth, barHeight, false);

  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
