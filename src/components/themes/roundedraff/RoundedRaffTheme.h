#pragma once

#include "components/themes/BaseTheme.h"

class GfxRenderer;

namespace RoundedRaffMetrics {
constexpr ThemeMetrics values = {.batteryWidth = 15,
                                 .batteryHeight = 12,
                                 .topPadding = 2,
                                 .batteryBarHeight = 14,
                                 .headerHeight = 30,
                                 .verticalSpacing = 6,
                                 .contentSidePadding = 10,
                                 .listRowHeight = 28,
                                 .listWithSubtitleRowHeight = 44,
                                 .menuRowHeight = 28,
                                 .menuSpacing = 4,
                                 .tabSpacing = 6,
                                 .tabBarHeight = 28,
                                 .scrollBarWidth = 3,
                                 .scrollBarRightOffset = 4,
                                 .homeTopPadding = 18,
                                 .homeCoverHeight = 110,
                                 .homeCoverTileHeight = 126,
                                 .homeRecentBooksCount = 1,
                                 .homeContinueReadingInMenu = true,
                                 .homeMenuTopOffset = 8,
                                 .buttonHintsHeight = 0,
                                 .sideButtonHintsWidth = 18,
                                 .progressBarHeight = 10,
                                 .progressBarMarginTop = 1,
                                 .statusBarHorizontalMargin = 4,
                                 .statusBarVerticalMargin = 8,
                                 .keyboardKeyWidth = 20,
                                 .keyboardKeyHeight = 24,
                                 .keyboardKeySpacing = 4,
                                 .keyboardBottomKeyHeight = 24,
                                 .keyboardBottomKeySpacing = 4,
                                 .keyboardBottomAligned = false,
                                 .keyboardCenteredText = false,
                                 .keyboardVerticalOffset = 0,
                                 .keyboardTextFieldWidthPercent = 92,
                                 .keyboardWidthPercent = 96,
                                 .keyboardKeyCornerRadius = 12};
}

class RoundedRaffTheme : public BaseTheme {
 public:
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title,
                  const char* subtitle = nullptr) const override;
  void drawTabBar(const GfxRenderer& renderer, Rect rect, const std::vector<TabInfo>& tabs,
                  bool selected) const override;
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
  void drawList(const GfxRenderer& renderer, Rect rect, int itemCount, int selectedIndex,
                const std::function<std::string(int index)>& rowTitle,
                const std::function<std::string(int index)>& rowSubtitle = nullptr,
                const std::function<UIIcon(int index)>& rowIcon = nullptr,
                const std::function<std::string(int index)>& rowValue = nullptr,
                bool highlightValue = false) const override;
  void drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                       const char* btn4) const override;
  bool homeMenuShowsContinueReading() const { return true; }
};
