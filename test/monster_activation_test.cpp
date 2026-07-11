#include <gtest/gtest.h>

#include "lighting.h"
#include "monster.h"
#include "player.h"

namespace devilution {
namespace {

class MonsterActivationTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
	}
};

TEST_F(MonsterActivationTest, MonsterActivationRadiusEqualsPlayerVisionPlus2)
{
	Player &player = Players[0];
	player._pLightRad = 8;

	// In cathedral (no suppression), player sees 8 tiles
	// Monster should activate at 8 + 2 = 10 tiles
	int activationRadius = GetMonsterActivationRadius(player, 1);
	EXPECT_EQ(activationRadius, 10);
}

TEST_F(MonsterActivationTest, MonsterActivationRadiusInCaves)
{
	Player &player = Players[0];
	player._pLightRad = 8;

	// In caves (80% suppression), player sees 6 tiles
	// Monster should activate at 6 + 2 = 8 tiles
	int activationRadius = GetMonsterActivationRadius(player, 9);
	EXPECT_EQ(activationRadius, 8);
}

TEST_F(MonsterActivationTest, MonsterActivationRadiusWithLightEquipment)
{
	Player &player = Players[0];
	player._pLightRad = 12; // Base 10 + 2 from equipment

	// In caves (80% suppression), player sees 10 tiles (8 + 2 equipment)
	// Monster should activate at 10 + 2 = 12 tiles
	int activationRadius = GetMonsterActivationRadius(player, 9);
	EXPECT_EQ(activationRadius, 12);
}

TEST_F(MonsterActivationTest, MonsterActivationRadiusInHell)
{
	Player &player = Players[0];
	player._pLightRad = 10;

	// In hell (60% suppression), player sees 6 tiles
	// Monster should activate at 6 + 2 = 8 tiles
	int activationRadius = GetMonsterActivationRadius(player, 13);
	EXPECT_EQ(activationRadius, 8);
}

TEST_F(MonsterActivationTest, MonsterActivationRadiusInCrypt)
{
	Player &player = Players[0];
	player._pLightRad = 10;

	// In crypt (50% suppression), player sees 5 tiles
	// Monster should activate at 5 + 2 = 7 tiles
	int activationRadius = GetMonsterActivationRadius(player, 21);
	EXPECT_EQ(activationRadius, 7);
}

} // namespace
} // namespace devilution
