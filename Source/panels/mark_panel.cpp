#include "mark_panel.hpp"
#include "engine/render/text_render.hpp"
#include "player.h"
#include "mastermark.h"
#include "control/control.hpp"

namespace devilution {

bool IsMarkPanelOpen = false;

void DrawMarkPanel(const Surface &out)
{
	if (MyPlayer == nullptr) return;

	const Point leftPanel = GetLeftPanel().position;
	const int x = leftPanel.x + 30;
	int y = leftPanel.y + 60;

	DrawString(out, "Master's Marks", Point { x, y }, { .flags = UiFlags::ColorWhitegold });
	y += 30;

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
