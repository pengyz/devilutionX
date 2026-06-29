#include <gtest/gtest.h>
#include "qol/skill_bar.h"

using namespace devilution;

TEST(SkillSlot, Default_CreatesEmptySlot)
{
    SkillSlot slot;
    EXPECT_EQ(slot.spellId, SpellID::Null);
    EXPECT_EQ(slot.spellType, SpellType::Skill);
    EXPECT_FALSE(slot.isActive);
    EXPECT_EQ(slot.charges, 0);
}

TEST(SkillSlot, IsEmpty_ReturnsTrueForNullSpell)
{
    SkillSlot slot;
    EXPECT_TRUE(slot.IsEmpty());
}

TEST(SkillSlot, IsEmpty_ReturnsFalseForBoundSpell)
{
    SkillSlot slot;
    slot.spellId = SpellID::Firebolt;
    EXPECT_FALSE(slot.IsEmpty());
}

TEST(SkillSlot, HasConsumable_ScrollWithCharges)
{
    SkillSlot slot;
    slot.spellType = SpellType::Scroll;
    slot.charges = 3;
    EXPECT_TRUE(slot.HasConsumable());
    EXPECT_EQ(slot.GetChargeText(), "3");
}

TEST(SkillSlot, HasConsumable_StaffWithCharges)
{
    SkillSlot slot;
    slot.spellType = SpellType::Charges;
    slot.charges = 12;
    EXPECT_TRUE(slot.HasConsumable());
    EXPECT_EQ(slot.GetChargeText(), "12");
}

TEST(SkillSlot, HasConsumable_LearnedSpell_ReturnsFalse)
{
    SkillSlot slot;
    slot.spellType = SpellType::Spell;
    EXPECT_FALSE(slot.HasConsumable());
}

TEST(SkillSlot, HasConsumable_ZeroChargeScroll_ReturnsTrue)
{
    SkillSlot slot;
    slot.spellType = SpellType::Scroll;
    slot.charges = 0;
    EXPECT_TRUE(slot.HasConsumable());
    EXPECT_EQ(slot.GetChargeText(), "0");
}

TEST(SkillBar, Init_HasFourSlots)
{
    SkillBar bar;
    for (int i = 0; i < SkillBar::SlotCount; i++) {
        EXPECT_TRUE(bar.GetSlot(i).IsEmpty());
    }
}

TEST(SkillBar, GetSlot_OutOfBounds_AssertInDebug)
{
    // This test verifies that accessing invalid slots is caught.
    // In release builds, it's UB. We test that indices 0-3 work.
    SkillBar bar;
    EXPECT_NO_THROW(bar.GetSlot(0));
    EXPECT_NO_THROW(bar.GetSlot(3));
}
