#pragma once

#include <cstdint>
#include <string>

#include "engine/point.hpp"
#include "tables/spelldat.h"

namespace devilution {

struct Player; // forward
struct Surface; // forward
using Point = PointOf<int>;

struct SkillSlot {
    SpellID spellId = SpellID::Null;
    SpellType spellType = SpellType::Skill;
    bool isActive = false;
    int charges = 0;

    [[nodiscard]] bool IsEmpty() const
    {
        return spellId == SpellID::Null;
    }

    [[nodiscard]] bool HasConsumable() const
    {
        return spellType == SpellType::Scroll
            || spellType == SpellType::Charges;
    }

    [[nodiscard]] std::string GetChargeText() const;
};

class SkillBar {
public:
    static constexpr int SlotCount = 4;

    void LoadFromPlayer(const Player &player);
    void Draw(const Surface &out, Point position);
    void Free();
    void MarkDirty();

    [[nodiscard]] const SkillSlot &GetSlot(int index) const;

private:
    SkillSlot slots_[SlotCount];
    bool dirty_ = true;
};

} // namespace devilution
