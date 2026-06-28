#pragma once

#include "engine/rectangle.hpp"
#include "panels/ui_panels.hpp"

namespace devilution {

extern int TotalSpMainPanelButtons;
extern int TotalMpMainPanelButtons;
extern int PanelPaddingHeight;
extern const char *const PanBtnStr[9];
extern const char *const PanBtnHotKey[9];
extern Rectangle SpellButtonRect;
extern Rectangle BeltRect;

void SetPanelObjectPosition(UiPanels panel, Rectangle &button);

} // namespace devilution
