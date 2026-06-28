// Data Integrity Tests
#include <gtest/gtest.h>
#include "tables/itemdat.h"
#include "player.h"
#include "mastermark.h"

namespace devilution {
namespace {

TEST(DataIntegrity, ProcFlagsUniqueBits)
{
    uint16_t flags[] = {
        PROC_FIREBALL_ONHIT, PROC_CHAINLIGHT_ONHIT,
        PROC_MANASTEAL_ONHIT, PROC_LIFESTEAL_ONHIT,
        PROC_FROSTNOVA_ONDAM, PROC_CONFUSE_ONHIT,
        PROC_BLOODLUST_ONKILL, PROC_VANISH_ONKILL,
        PROC_CRITNEXT_ONKILL, PROC_MANASHIELD_ONDAM,
        PROC_HASTE_ONDAM, PROC_THORNS_ONDAM,
    };
    uint16_t all = 0;
    for (auto f : flags) { EXPECT_EQ(all & f, 0); all |= f; }
}

TEST(DataIntegrity, MarkCount9)   { EXPECT_EQ(MarkCount, size_t(9)); }
TEST(DataIntegrity, InvGrid60)    { EXPECT_EQ(InventoryGridCells, 60); }
TEST(DataIntegrity, ItemDefaults) { Item t; EXPECT_EQ(t._iProcFlags, 0); EXPECT_EQ(t._iSetId, -1); }

} // namespace
} // namespace devilution
