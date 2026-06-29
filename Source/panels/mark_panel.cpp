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
#include "utils/language.h"

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
	DrawString(out, _("Master's Marks"),
	    Point { leftPanel.x + PanelPaddingLeft, leftPanel.y + 4 },
	    { .flags = UiFlags::ColorWhitegold });

	// Close button (X) in top-right corner
	constexpr int CloseRectX = SidePanelSize.width - CloseButtonMargin - CloseButtonSize;
	const int closeX = leftPanel.x + CloseRectX;
	const int closeY = leftPanel.y + CloseButtonMargin;
	FillRect(out, closeX, closeY, CloseButtonSize, CloseButtonSize, PAL16_BLUE + 8);
	DrawString(out, _("X"),
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
			DrawString(out, std::string(_("Slot ")) + std::to_string(slot + 1) + ": " + std::string(_("Empty")),
			    Point { x, y }, { .flags = UiFlags::ColorWhite });
			y += 18;
			continue;
		}

		const auto &def = markDefs[static_cast<size_t>(mid)];
		std::string label = std::string(slot == 0 ? _("[Primary] ") : _("[Secondary] ")) + std::string(_(def.name));
		DrawString(out, label, Point { x, y }, { .flags = UiFlags::ColorWhitegold });
		y += 16;
		DrawString(out, std::string(_("Core: ")) + std::string(_(def.coreMechanic)), Point { x + 10, y }, { .flags = UiFlags::ColorWhite });
		y += 14;
		DrawString(out, _(def.description), Point { x + 10, y }, { .flags = UiFlags::ColorWhite });
		y += 16;

		auto &state = GetMarkState(*MyPlayer, mid);
		const char *rarityNames[] = { N_("Common"), N_("Rare"), N_("Legendary") };
		for (int s = 0; s < 3; s++) {
			bool locked = (slot == 1 && s >= 1);
			std::string slotLabel = std::string(_(rarityNames[s]));

			std::string lineA = slotLabel + " A: " + std::string(_(def.slotDescA[s]));
			uint8_t choice = state.slots[s].choice;
			if (locked)
				lineA += " (" + std::string(_("Locked")) + ")";
			else if (choice == 1)
				lineA += " " + std::string(_("[SOCKETED]"));

			DrawString(out, lineA, Point { x + 10, y },
			    { .flags = (choice == 1 && !locked) ? UiFlags::ColorWhitegold : UiFlags::ColorWhite });
			y += 14;

			std::string lineB = slotLabel + " B: " + std::string(_(def.slotDescB[s]));
			if (locked)
				lineB += " (" + std::string(_("Locked")) + ")";
			else if (choice == 2)
				lineB += " " + std::string(_("[SOCKETED]"));

			DrawString(out, lineB, Point { x + 10, y },
			    { .flags = (choice == 2 && !locked) ? UiFlags::ColorWhitegold : UiFlags::ColorWhite });
			y += 14;
		}
		y += 6;
	}

	y += 10;
	DrawString(out, _("Other Marks:"), Point { x, y }, { .flags = UiFlags::ColorWhitegold });
	y += 20;
	for (size_t i = 0; i < MarkCount; i++) {
		MasterMarkId mid = static_cast<MasterMarkId>(i);
		if (HasMark(*MyPlayer, mid) && !HasActiveMark(*MyPlayer, mid)) {
			DrawString(out, _(markDefs[i].name), Point { x + 10, y }, { .flags = UiFlags::ColorWhite });
			y += 18;
		}
	}
}

} // namespace devilution
