#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string_view>
#include <unordered_map>

#include "tables/misdat.h"

using namespace devilution;
using ::testing::NotNull;
using ::testing::IsNull;

namespace {

// Custom missile function for testing registry
void TestCustomAddFn(Missile &, AddMissileParameter &)
{
	// no-op for test
}

void TestCustomProcessFn(Missile &)
{
	// no-op for test
}

// List of all built-in AddFn names from misdat.tsv
const char *g_builtinAddFnNames[] = {
	"AddOpenNest", "AddRuneOfFire", "AddRuneOfLight", "AddRuneOfNova",
	"AddRuneOfImmolation", "AddRuneOfStone", "AddReflect", "AddBerserk",
	"AddHorkSpawn", "AddJester", "AddStealPotions", "AddStealMana",
	"AddSpectralArrow", "AddWarp", "AddLightningWall", "AddBigExplosion",
	"AddImmolation", "AddLightningBow", "AddMana", "AddMagi",
	"AddRingOfFire", "AddSearch", "AddChargedBoltBow",
	"AddElementalArrow", "AddArrow", "AddPhasing", "AddFirebolt",
	"AddMagmaBall", "AddTeleport", "AddNovaBall", "AddFireWall",
	"AddFireball", "AddLightningControl", "AddLightning",
	"AddMissileExplosion", "AddWeaponExplosion", "AddTownPortal",
	"AddFlashBottom", "AddFlashTop", "AddManaShield", "AddFlameWave",
	"AddGuardian", "AddChainLightning", "AddRhino",
	"AddGenericMagicMissile", "AddAcid", "AddAcidPuddle",
	"AddStoneCurse", "AddGolem", "AddApocalypseBoom", "AddHealing",
	"AddHealOther", "AddElemental", "AddIdentify", "AddWallControl",
	"AddInfravision", "AddFlameWaveControl", "AddNova", "AddRage",
	"AddItemRepair", "AddStaffRecharge", "AddTrapDisarm", "AddApocalypse",
	"AddInferno", "AddInfernoControl", "AddChargedBolt", "AddHolyBolt",
	"AddResurrect", "AddResurrectBeam", "AddTelekinesis", "AddBoneSpirit",
	"AddRedPortal", "AddDiabloApocalypse",
};

constexpr size_t kNumBuiltinAddFnNames = sizeof(g_builtinAddFnNames) / sizeof(g_builtinAddFnNames[0]);

// Select subset of built-in ProcessFn names
const char *g_builtinProcessFnNames[] = {
	"ProcessElementalArrow", "ProcessArrow",
	"ProcessGenericProjectile", "ProcessNovaBall",
	"ProcessAcidPuddle", "ProcessFireWall", "ProcessFireball",
	"ProcessHorkSpawn", "ProcessRune",
};

constexpr size_t kNumBuiltinProcessFnNames = sizeof(g_builtinProcessFnNames) / sizeof(g_builtinProcessFnNames[0]);

} // namespace

TEST(MissileRegistryTest, AllBuiltinAddFnNamesResolve)
{
	for (size_t i = 0; i < kNumBuiltinAddFnNames; i++) {
		auto result = ParseMissileAddFn(g_builtinAddFnNames[i]);
		ASSERT_TRUE(result.has_value())
		    << "Failed to resolve built-in AddFn: " << g_builtinAddFnNames[i];
		EXPECT_NE(*result, nullptr)
		    << "Built-in AddFn " << g_builtinAddFnNames[i] << " resolved to nullptr";
	}
}

TEST(MissileRegistryTest, AllBuiltinProcessFnNamesResolve)
{
	for (size_t i = 0; i < kNumBuiltinProcessFnNames; i++) {
		auto result = ParseMissileProcessFn(g_builtinProcessFnNames[i]);
		ASSERT_TRUE(result.has_value())
		    << "Failed to resolve built-in ProcessFn: " << g_builtinProcessFnNames[i];
		EXPECT_NE(*result, nullptr)
		    << "Built-in ProcessFn " << g_builtinProcessFnNames[i] << " resolved to nullptr";
	}
}

TEST(MissileRegistryTest, RegisterCustomAddFn)
{
	RegisterMissileAddFn("TestCustomAdd", TestCustomAddFn);

	auto result = ParseMissileAddFn("TestCustomAdd");
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, reinterpret_cast<MissileData::AddFn>(TestCustomAddFn));
}

TEST(MissileRegistryTest, RegisterCustomProcessFn)
{
	RegisterMissileProcessFn("TestCustomProcess", TestCustomProcessFn);

	auto result = ParseMissileProcessFn("TestCustomProcess");
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, reinterpret_cast<MissileData::ProcessFn>(TestCustomProcessFn));
}

TEST(MissileRegistryTest, EmptyStringReturnsNullptr)
{
	auto result = ParseMissileAddFn("");
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, nullptr);
}

TEST(MissileRegistryTest, UnknownNameReturnsError)
{
	auto result = ParseMissileAddFn("NonExistentMissileFnName_XYZ123");
	EXPECT_FALSE(result.has_value());
}
