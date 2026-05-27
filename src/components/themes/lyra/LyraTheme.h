#pragma once

#include "components/themes/BaseTheme.h"

class GfxRenderer;

// Lyra theme metrics (zero runtime cost)
namespace LyraMetrics {
constexpr ThemeMetrics values = {.batteryWidth = 16,
                                 .batteryHeight = 12,
                                 .topPadding = 3,
                                 .batteryBarHeight = 18,
                                 .headerHeight = 42,
                                 .verticalSpacing = 8,
                                 .contentSidePadding = 10,
                                 .listRowHeight = 28,
                                 .listWithSubtitleRowHeight = 42,
                                 .menuRowHeight = 42,
                                 .menuSpacing = 6,
                                 .tabSpacing = 6,
                                 .tabBarHeight = 28,
                                 .scrollBarWidth = 3,
                                 .scrollBarRightOffset = 4,
                                 .homeTopPadding = 22,
                                 .homeCoverHeight = 118,
                                 .homeCoverTileHeight = 134,
                                 .homeRecentBooksCount = 1,
                                 .homeContinueReadingInMenu = false,
                                 .homeMenuTopOffset = 8,
                                 .buttonHintsHeight = 0,
                                 .sideButtonHintsWidth = 18,
                                 .progressBarHeight = 10,
                                 .progressBarMarginTop = 1,
                                 .statusBarHorizontalMargin = 4,
                                 .statusBarVerticalMargin = 8,
                                 .keyboardKeyWidth = 22,
                                 .keyboardKeyHeight = 26,
                                 .keyboardKeySpacing = 1,
                                 .keyboardBottomKeyHeight = 24,
                                 .keyboardBottomKeySpacing = 4,
                                 .keyboardBottomAligned = true,
                                 .keyboardCenteredText = false,
                                 .keyboardVerticalOffset = -2,
                                 .keyboardTextFieldWidthPercent = 92,
                                 .keyboardWidthPercent = 96,
                                 .keyboardKeyCornerRadius = 6};
}

class LyraTheme : public BaseTheme {
 public:
  // Component drawing methods
  //   void drawProgressBar(const GfxRenderer& renderer, Rect rect, size_t current, size_t total) override;
  void drawBatteryLeft(const GfxRenderer& renderer, Rect rect, bool showPercentage = true) const override;
  void drawBatteryRight(const GfxRenderer& renderer, Rect rect, bool showPercentage = true) const override;
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle) const override;
  void drawSubHeader(const GfxRenderer& renderer, Rect rect, const char* label,
                     const char* rightLabel = nullptr) const override;
  void drawTabBar(const GfxRenderer& renderer, Rect rect, const std::vector<TabInfo>& tabs,
                  bool selected) const override;
  void drawList(const GfxRenderer& renderer, Rect rect, int itemCount, int selectedIndex,
                const std::function<std::string(int index)>& rowTitle,
                const std::function<std::string(int index)>& rowSubtitle,
                const std::function<UIIcon(int index)>& rowIcon, const std::function<std::string(int index)>& rowValue,
                bool highlightValue) const override;
  void drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                       const char* btn4) const override;
  void drawSideButtonHints(const GfxRenderer& renderer, const char* topBtn, const char* bottomBtn) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           const int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;
  void drawEmptyRecents(const GfxRenderer& renderer, const Rect rect) const;
  Rect drawPopup(const GfxRenderer& renderer, const char* message) const override;
  void fillPopupProgress(const GfxRenderer& renderer, const Rect& layout, const int progress) const override;
  bool showsFileIcons() const override { return true; }
};
