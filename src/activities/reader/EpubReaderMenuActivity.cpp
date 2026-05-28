#include "EpubReaderMenuActivity.h"

#include <algorithm>

#include <GfxRenderer.h>
#include <I18n.h>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

EpubReaderMenuActivity::EpubReaderMenuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                               const std::string& title, const int currentPage, const int totalPages,
                                               const int bookProgressPercent, const uint8_t currentOrientation,
                                               const bool hasFootnotes)
    : Activity("EpubReaderMenu", renderer, mappedInput),
      menuItems(buildMenuItems(hasFootnotes)),
      title(title),
      pendingOrientation(currentOrientation),
      currentPage(currentPage),
      totalPages(totalPages),
      bookProgressPercent(bookProgressPercent) {}

std::vector<EpubReaderMenuActivity::MenuItem> EpubReaderMenuActivity::buildMenuItems(bool hasFootnotes) {
  std::vector<MenuItem> items;
  items.reserve(10);
  items.push_back({MenuAction::SELECT_CHAPTER, StrId::STR_SELECT_CHAPTER});
  if (hasFootnotes) {
    items.push_back({MenuAction::FOOTNOTES, StrId::STR_FOOTNOTES});
  }
  items.push_back({MenuAction::ROTATE_SCREEN, StrId::STR_ORIENTATION});
  items.push_back({MenuAction::AUTO_PAGE_TURN, StrId::STR_AUTO_TURN_PAGES_PER_MIN});
  items.push_back({MenuAction::GO_TO_PERCENT, StrId::STR_GO_TO_PERCENT});
  items.push_back({MenuAction::SCREENSHOT, StrId::STR_SCREENSHOT_BUTTON});
  items.push_back({MenuAction::DISPLAY_QR, StrId::STR_DISPLAY_QR});
  items.push_back({MenuAction::GO_HOME, StrId::STR_GO_HOME_BUTTON});
  items.push_back({MenuAction::SYNC, StrId::STR_SYNC_PROGRESS});
  items.push_back({MenuAction::DELETE_CACHE, StrId::STR_DELETE_CACHE});
  return items;
}

void EpubReaderMenuActivity::onEnter() {
  Activity::onEnter();
  requestUpdate();
}

void EpubReaderMenuActivity::onExit() { Activity::onExit(); }

void EpubReaderMenuActivity::loop() {
  // Handle navigation
  buttonNavigator.onNext([this] {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, static_cast<int>(menuItems.size()));
    requestUpdate();
  });

  buttonNavigator.onPrevious([this] {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, static_cast<int>(menuItems.size()));
    requestUpdate();
  });

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    const auto selectedAction = menuItems[selectedIndex].action;
    if (selectedAction == MenuAction::ROTATE_SCREEN) {
      // Cycle orientation preview locally; actual rotation happens on menu exit.
      pendingOrientation = (pendingOrientation + 1) % orientationLabels.size();
      requestUpdate();
      return;
    }

    if (selectedAction == MenuAction::AUTO_PAGE_TURN) {
      selectedPageTurnOption = (selectedPageTurnOption + 1) % pageTurnLabels.size();
      requestUpdate();
      return;
    }

    setResult(MenuResult{static_cast<int>(selectedAction), pendingOrientation, selectedPageTurnOption});
    finish();
    return;
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    ActivityResult result;
    result.isCancelled = true;
    result.data = MenuResult{-1, pendingOrientation, selectedPageTurnOption};
    setResult(std::move(result));
    finish();
    return;
  }
}

void EpubReaderMenuActivity::render(RenderLock&&) {
  renderer.clearScreen();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const auto orientation = renderer.getOrientation();
  // Landscape orientation: button hints are drawn along a vertical edge, so we
  // reserve a horizontal gutter to prevent overlap with menu content.
  const bool isLandscapeCw = orientation == GfxRenderer::Orientation::LandscapeClockwise;
  const bool isLandscapeCcw = orientation == GfxRenderer::Orientation::LandscapeCounterClockwise;
  // Inverted portrait: button hints appear near the logical top, so we reserve
  // vertical space to keep the header and list clear.
  const bool isPortraitInverted = orientation == GfxRenderer::Orientation::PortraitInverted;
  const int hintGutterWidth = (isLandscapeCw || isLandscapeCcw) ? 30 : 0;
  // Landscape CW places hints on the left edge; CCW keeps them on the right.
  const int contentX = isLandscapeCw ? hintGutterWidth : 0;
  const int contentWidth = pageWidth - hintGutterWidth;
  const int hintGutterHeight = isPortraitInverted ? 50 : 0;
  const int contentY = hintGutterHeight;
  const bool useLyraCompactLayout = SETTINGS.uiTheme == CrossPointSettings::UI_THEME::LYRA;

  if (useLyraCompactLayout) {
    constexpr int kHeaderSidePadding = 12;
    constexpr int kHeaderTopPadding = 12;
    constexpr int kHeaderBottomGap = 8;
    constexpr int kMenuRowHeight = 24;
    constexpr int kMenuRowGap = 3;
    constexpr int kMenuRowRadius = 6;
    constexpr int kMenuTextPadding = 10;
    constexpr int kMenuBottomPadding = 8;
    constexpr int kMenuValueGap = 10;
    constexpr int kScrollBarWidth = 3;
    constexpr int kScrollBarGap = 4;
    constexpr int kDividerThickness = 2;
    constexpr int kHeaderValueGap = 3;

    auto titleLines = renderer.wrappedText(UI_10_FONT_ID, title.c_str(),
                                           std::max(40, contentWidth - kHeaderSidePadding * 2), 2,
                                           EpdFontFamily::BOLD);
    if (titleLines.empty()) {
      titleLines.push_back("");
    }

    std::string progressLine;
    if (totalPages > 0) {
      progressLine = std::string(tr(STR_CHAPTER_PREFIX)) + std::to_string(currentPage) + "/" +
                     std::to_string(totalPages) + std::string(tr(STR_PAGES_SEPARATOR));
    }
    progressLine += std::string(tr(STR_BOOK_PREFIX)) + std::to_string(bookProgressPercent) + "%";

    const int titleLineHeight = renderer.getLineHeight(UI_10_FONT_ID);
    const int metaLineHeight = renderer.getLineHeight(SMALL_FONT_ID);
    int headerY = contentY + kHeaderTopPadding;
    for (const auto& line : titleLines) {
      const int lineWidth = renderer.getTextWidth(UI_10_FONT_ID, line.c_str(), EpdFontFamily::BOLD);
      const int lineX = contentX + std::max(0, (contentWidth - lineWidth) / 2);
      renderer.drawText(UI_10_FONT_ID, lineX, headerY, line.c_str(), true, EpdFontFamily::BOLD);
      headerY += titleLineHeight;
    }

    const int progressY = headerY + kHeaderValueGap;
    const int progressWidth = renderer.getTextWidth(SMALL_FONT_ID, progressLine.c_str());
    const int progressX = contentX + std::max(0, (contentWidth - progressWidth) / 2);
    renderer.drawText(SMALL_FONT_ID, progressX, progressY, progressLine.c_str(), true);

    const int dividerY = progressY + metaLineHeight + kHeaderBottomGap;
    renderer.drawLine(contentX + kHeaderSidePadding, dividerY, contentX + contentWidth - kHeaderSidePadding, dividerY,
                      kDividerThickness, true);

    const int listTop = dividerY + 8;
    const int listBottom = pageHeight - kMenuBottomPadding;
    const int scrollAreaWidth = (static_cast<int>(menuItems.size()) > 0) ? (kScrollBarWidth + kScrollBarGap) : 0;
    const int listContentWidth = std::max(0, contentWidth - kHeaderSidePadding * 2 - scrollAreaWidth);
    const int pageItems =
        std::max(1, (listBottom - listTop + kMenuRowGap) / std::max(1, kMenuRowHeight + kMenuRowGap));
    const int pageStartIndex = selectedIndex / pageItems * pageItems;
    const int pageEndIndex = std::min(static_cast<int>(menuItems.size()), pageStartIndex + pageItems);
    const int totalPagesForMenu = (static_cast<int>(menuItems.size()) + pageItems - 1) / pageItems;

    if (totalPagesForMenu > 1) {
      const int visibleHeight =
          (pageEndIndex - pageStartIndex) * kMenuRowHeight + std::max(0, pageEndIndex - pageStartIndex - 1) * kMenuRowGap;
      const int scrollTrackX = contentX + contentWidth - kHeaderSidePadding - kScrollBarWidth;
      const int scrollTrackY = listTop;
      const int scrollTrackHeight = std::max(1, visibleHeight);
      const int thumbHeight = std::max(kMenuRowHeight, (scrollTrackHeight * pageItems) / static_cast<int>(menuItems.size()));
      const int currentPage = selectedIndex / pageItems;
      const int maxThumbTravel = std::max(0, scrollTrackHeight - thumbHeight);
      const int thumbY = scrollTrackY +
                         ((totalPagesForMenu > 1) ? (maxThumbTravel * currentPage) / (totalPagesForMenu - 1) : 0);

      renderer.drawLine(scrollTrackX, scrollTrackY, scrollTrackX, scrollTrackY + scrollTrackHeight, true);
      renderer.fillRect(scrollTrackX - kScrollBarWidth + 1, thumbY, kScrollBarWidth, thumbHeight, true);
    }

    for (int i = pageStartIndex; i < pageEndIndex; ++i) {
      const int visualIndex = i - pageStartIndex;
      const int rowY = listTop + visualIndex * (kMenuRowHeight + kMenuRowGap);
      const bool isSelected = i == selectedIndex;
      const int rowX = contentX + kHeaderSidePadding;

      if (isSelected) {
        renderer.fillRoundedRect(rowX, rowY, listContentWidth, kMenuRowHeight, kMenuRowRadius, Color::LightGray);
      }

      std::string valueText;
      if (menuItems[i].action == MenuAction::ROTATE_SCREEN) {
        valueText = I18N.get(orientationLabels[pendingOrientation]);
      } else if (menuItems[i].action == MenuAction::AUTO_PAGE_TURN) {
        valueText = pageTurnLabels[selectedPageTurnOption];
      }

      const int valueWidth =
          valueText.empty() ? 0 : renderer.getTextWidth(SMALL_FONT_ID, valueText.c_str(), EpdFontFamily::REGULAR);
      const int labelMaxWidth =
          std::max(20, listContentWidth - kMenuTextPadding * 2 - (valueText.empty() ? 0 : valueWidth + kMenuValueGap));
      auto label = renderer.truncatedText(UI_10_FONT_ID, I18N.get(menuItems[i].labelId), labelMaxWidth,
                                          EpdFontFamily::REGULAR);
      const int labelY = rowY + std::max(0, (kMenuRowHeight - renderer.getLineHeight(UI_10_FONT_ID)) / 2);

      renderer.drawText(UI_10_FONT_ID, rowX + kMenuTextPadding, labelY, label.c_str(), true, EpdFontFamily::REGULAR);

      if (!valueText.empty()) {
        const int valueY = rowY + std::max(0, (kMenuRowHeight - metaLineHeight) / 2);
        const int valueX = rowX + listContentWidth - kMenuTextPadding - valueWidth;
        renderer.drawText(SMALL_FONT_ID, valueX, valueY, valueText.c_str(), true);
      }
    }

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

    renderer.displayBuffer();
    return;
  }

  // Title
  const std::string truncTitle =
      renderer.truncatedText(UI_12_FONT_ID, title.c_str(), contentWidth - 40, EpdFontFamily::BOLD);
  // Manual centering so we can respect the content gutter.
  const int titleX =
      contentX + (contentWidth - renderer.getTextWidth(UI_12_FONT_ID, truncTitle.c_str(), EpdFontFamily::BOLD)) / 2;
  renderer.drawText(UI_12_FONT_ID, titleX, 15 + contentY, truncTitle.c_str(), true, EpdFontFamily::BOLD);

  // Progress summary
  std::string progressLine;
  if (totalPages > 0) {
    progressLine = std::string(tr(STR_CHAPTER_PREFIX)) + std::to_string(currentPage) + "/" +
                   std::to_string(totalPages) + std::string(tr(STR_PAGES_SEPARATOR));
  }
  progressLine += std::string(tr(STR_BOOK_PREFIX)) + std::to_string(bookProgressPercent) + "%";
  renderer.drawCenteredText(UI_10_FONT_ID, 45, progressLine.c_str());

  // Menu Items
  const int startY = 75 + contentY;
  constexpr int lineHeight = 30;

  for (size_t i = 0; i < menuItems.size(); ++i) {
    const int displayY = startY + (i * lineHeight);
    const bool isSelected = (static_cast<int>(i) == selectedIndex);

    if (isSelected) {
      // Highlight only the content area so we don't paint over hint gutters.
      renderer.fillRect(contentX, displayY, contentWidth - 1, lineHeight, true);
    }

    renderer.drawText(UI_10_FONT_ID, contentX + 20, displayY, I18N.get(menuItems[i].labelId), !isSelected);

    if (menuItems[i].action == MenuAction::ROTATE_SCREEN) {
      // Render current orientation value on the right edge of the content area.
      const char* value = I18N.get(orientationLabels[pendingOrientation]);
      const auto width = renderer.getTextWidth(UI_10_FONT_ID, value);
      renderer.drawText(UI_10_FONT_ID, contentX + contentWidth - 20 - width, displayY, value, !isSelected);
    }

    if (menuItems[i].action == MenuAction::AUTO_PAGE_TURN) {
      // Render current page turn value on the right edge of the content area.
      const auto value = pageTurnLabels[selectedPageTurnOption];
      const auto width = renderer.getTextWidth(UI_10_FONT_ID, value);
      renderer.drawText(UI_10_FONT_ID, contentX + contentWidth - 20 - width, displayY, value, !isSelected);
    }
  }

  // Footer / Hints
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
