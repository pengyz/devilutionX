#include "mark_panel.hpp"

#include <cstdint>

#include "control/control.hpp"
#include "effects.h"
#include "engine/palette.h"
#include "engine/render/primitive_render.hpp"
#include "engine/render/text_render.hpp"
#include "engine/size.hpp"
#include "mastermark.h"
#include "player.h"

namespace devilution {

bool IsMarkPanelOpen = false;

namespace {
constexpr int TitleBarHeight = 25;
constexpr int CloseButtonSize = 20;
constexpr int CloseButtonMargin = 4;
constexpr int ContentPaddingX = 20;
constexpr int ContentStartY = 40;
constexpr int PanelPaddingLeft = 30;
} // namespace

Rectangle GetMarkPanelCloseRect()
{
	const Point leftPanel = GetLeftPanel().position;
	const int closeX = leftPanel.x + SidePanelSize.width - CloseButtonMargin - CloseButtonSize;
	const int closeY = leftPanel.y + CloseButtonMargin;
	return Rectangle { Point { closeX, closeY }, Size { CloseButtonSize, CloseButtonSize } };
}

void CheckMarkPanelButton()
{
	if (!MarkPanelFlag) return;
	if (!GetMarkPanelCloseRect().contains(MousePosition)) return;
	MarkPanelFlag = false;
	PlaySFX(SfxID::MenuSelect);
}

void DrawMarkPanel(const Surface &out)
{
	if (MyPlayer == nullptr) return;

	const Point leftPanel = GetLeftPanel().position;
	const int panelWidth = SidePanelSize.width;
	const int panelHeight = SidePanelSize.height;

	// Dark background for the entire panel
	DrawHalfTransparentRectTo(out, leftPanel.x, leftPanel.y, panelWidth, panelHeight);

	// Title bar background (slightly darker/blue tint)
	FillRect(out, leftPanel.x, leftPanel.y, panelWidth, TitleBarHeight, PAL16_BLUE + 14);

	// Title text centered in title bar
	DrawString(out, "Master's Marks",
	    Point { leftPanel.x + PanelPaddingLeft, leftPanel.y + 4 },
	    { .flags = UiFlags::ColorWhitegold });

	// Close button (X) in top-right corner
	constexpr int CloseRectX = SidePanelSize.width - CloseButtonMargin - CloseButtonSize;
	const int closeX = leftPanel.x + CloseRectX;
	const int closeY = leftPanel.y + CloseButtonMargin;
	// Draw a subtle close button background
	FillRect(out, closeX, closeY, CloseButtonSize, CloseButtonSize, PAL16_BLUE + 8);
	DrawString(out, "X",
	    Point { closeX + 5, closeY + 2 },
	    { .flags = UiFlags::ColorWhite });

	// Draw a separator line below the title bar
	DrawHorizontalLine(out, Point { leftPanel.x, leftPanel.y + TitleBarHeight }, panelWidth, PAL16_BLUE + 8);

	// Content area
	const int x = leftPanel.x + PanelPaddingLeft;
	int y = leftPanel.y + ContentStartY;

	for (int slot = 0; slot < 2; slot++) {
		MasterMarkId mid = MyPlayer->activeMarks[slot];
		if (mid == MasterMarkId::COUNT) {
			DrawString(out, "Slot " + std::to_string(slot + 1) + ": Empty",
			    Point { x, y }, { .flags = UiFlags::ColorWhite });
			y += 18;
			continue;
		}

		const auto &def = markDefs[static_cast<size_t>(mid)];
		std::string label = (slot == 0 ? "[Primary] " : "[Secondary] ") + std::string(def.name);
		DrawString(out, label, Point { x, y }, { .flags = UiFlags::ColorWhitegold });
		y += 20;
		DrawString(out, def.description, Point { x + 10, y }, { .flags = UiFlags::ColorWhite });
		y += 25;

		auto &state = GetMarkState(*MyPlayer, mid);
		const char *rarityNames[] = { "Common", "Rare", "Legendary" };
		for (int s = 0; s < 3; s++) {
			bool locked = (slot == 1 && s >= 1);
			std::string info = std::string(rarityNames[s]) + ": ";
			if (locked) info += "Locked";
			else if (state.slots[s].socketed) info += (state.slots[s].choice == 1) ? "Path A" : "Path B";
			else info += "Empty";
			DrawString(out, info, Point { x + 20, y }, { .flags = UiFlags::ColorWhite });
			y += 16;
		}
		y += 10;
	}

	y += 10;
	DrawString(out, "Other Marks:", Point { x, y }, { .flags = UiFlags::ColorWhitegold });
	y += 20;
	for (size_t i = 0; i < MarkCount; i++) {
		MasterMarkId mid = static_cast<MasterMarkId>(i);
		if (HasMark(*MyPlayer, mid) && !HasActiveMark(*MyPlayer, mid)) {
			DrawString(out, markDefs[i].name, Point { x + 10, y }, { .flags = UiFlags::ColorWhite });
			y += 18;
		}
	}
}

} // namespace devilution
