#include "qol/skill_bar.h"

#include "control/control.hpp"
#include "engine/render/primitive_render.hpp"
#include "engine/render/text_render.hpp"
#include "panels/spell_icons.hpp"
#include "player.h"
#include "utils/language.h"

namespace devilution {

std::string SkillSlot::GetChargeText() const
{
	return std::to_string(charges);
}

void SkillBar::LoadFromPlayer(const Player &player)
{
	// Quick check: if hotkeys and ready spell haven't changed, skip copy
	bool changed = false;
	for (int i = 0; i < SlotCount; i++) {
		if (slots_[i].spellId != player._pSplHotKey[i]
		    || slots_[i].spellType != player._pSplTHotKey[i]) {
			changed = true;
			break;
		}
	}
	bool activeChanged = false;
	for (int i = 0; i < SlotCount; i++) {
		bool nowActive = (player._pRSpell == player._pSplHotKey[i])
		              && (player._pRSplType == player._pSplTHotKey[i]);
		if (slots_[i].isActive != nowActive) {
			activeChanged = true;
			break;
		}
	}
	if (!changed && !activeChanged)
		return;

	for (int i = 0; i < SlotCount; i++) {
		if (player._pSplHotKey[i] != SpellID::Null) {
			slots_[i].spellId = player._pSplHotKey[i];
			slots_[i].spellType = player._pSplTHotKey[i];
			slots_[i].isActive = (player._pRSpell == player._pSplHotKey[i])
			                  && (player._pRSplType == player._pSplTHotKey[i]);
			slots_[i].charges = 0;
		} else {
			slots_[i] = SkillSlot {};
			slots_[i].isActive = false;
		}
	}
}

void SkillBar::Draw(const Surface &out, Point basePosition)
{
	if (!AreSmallSpellIconsLoaded())
		return;

	constexpr int slotSize = 28;
	constexpr int slotSpacing = 6;
	constexpr int startX = 200;
	constexpr int startY = 50;

	for (int i = 0; i < SlotCount; i++) {
		const int x = basePosition.x + startX + i * (slotSize + slotSpacing);
		const int y = basePosition.y + startY;

		// Draw slot background
		DrawHalfTransparentRectTo(out, x, y, slotSize, slotSize);

		if (slots_[i].IsEmpty())
			continue;

		// Draw spell icon
		DrawSmallSpellIcon(out, { x, y + slotSize - 1 }, slots_[i].spellId);

		// Draw active highlight border
		if (slots_[i].isActive) {
			DrawSmallSpellIconBorder(out, { x, y + slotSize - 1 });
		}
	}
}

void SkillBar::Free()
{
}

const SkillSlot &SkillBar::GetSlot(int index) const
{
	return slots_[index];
}

} // namespace devilution
