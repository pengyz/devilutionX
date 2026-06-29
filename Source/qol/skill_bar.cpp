#include "qol/skill_bar.h"

#include "player.h"

namespace devilution {

std::string SkillSlot::GetChargeText() const
{
    return std::to_string(charges);
}

void SkillBar::LoadFromPlayer(const Player &player)
{
    for (int i = 0; i < SlotCount; i++) {
        if (player._pSplHotKey[i] != SpellID::Null) {
            slots_[i].spellId = player._pSplHotKey[i];
            slots_[i].spellType = player._pSplTHotKey[i];
            slots_[i].isActive = (player._pRSpell == player._pSplHotKey[i])
                              && (player._pRSplType == player._pSplTHotKey[i]);
            slots_[i].charges = 0;
            // Charges will be populated by rendering code when needed
        } else {
            slots_[i] = SkillSlot {};
            slots_[i].isActive = false;
        }
    }
}

void SkillBar::Draw(const Surface & /*out*/, Point /*position*/)
{
    // Rendering implemented in Task 9 after panel layout is finalized
}

void SkillBar::Free()
{
    // Free any graphics resources (none yet)
}

const SkillSlot &SkillBar::GetSlot(int index) const
{
    return slots_[index];
}

} // namespace devilution
