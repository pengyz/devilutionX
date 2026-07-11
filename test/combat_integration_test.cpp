#include <gtest/gtest.h>

#include "lighting.h"
#include "monster.h"
#include "player.h"

namespace devilution {
namespace {

class CombatIntegrationTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
	}
};

TEST_F(CombatIntegrationTest, MonsterActivationInDarkArea)
{
	// Set up player in a dark area
	Player &player = Players[0];
	player._pLightRad = 10;

	// Monster should activate before player can see it
	int playerVision = GetEffectiveLightRadius(player, 21); // Crypt
	int monsterActivation = GetMonsterActivationRadius(player, 21);

	EXPECT_GT(monsterActivation, playerVision);
	EXPECT_EQ(monsterActivation, playerVision + 2);
}

TEST_F(CombatIntegrationTest, LightEquipmentReducesDelta)
{
	// Player with light equipment
	Player &player = Players[0];
	player._pLightRad = 12; // Base 10 + 2 from equipment

	int playerVision = GetEffectiveLightRadius(player, 21);
	int monsterActivation = GetMonsterActivationRadius(player, 21);

	// Delta should be smaller with equipment
	int delta = monsterActivation - playerVision;
	EXPECT_EQ(delta, 2);
}

TEST_F(CombatIntegrationTest, DifferentLevelsHaveDifferentSuppression)
{
	Player &player = Players[0];
	player._pLightRad = 10;

	// Cathedral: no suppression
	int cathedralVision = GetEffectiveLightRadius(player, 1);
	EXPECT_EQ(cathedralVision, 10);

	// Caves: 80% suppression
	int cavesVision = GetEffectiveLightRadius(player, 9);
	EXPECT_EQ(cavesVision, 8);

	// Hell: 60% suppression
	int hellVision = GetEffectiveLightRadius(player, 13);
	EXPECT_EQ(hellVision, 6);

	// Crypt: 50% suppression
	int cryptVision = GetEffectiveLightRadius(player, 21);
	EXPECT_EQ(cryptVision, 5);
}

TEST_F(CombatIntegrationTest, MonsterActivationFollowsPlayerVision)
{
	Player &player = Players[0];
	player._pLightRad = 10;

	// Monster activation should always be player vision + 2
	for (int level = 1; level <= 24; level++) {
		int playerVision = GetEffectiveLightRadius(player, level);
		int monsterActivation = GetMonsterActivationRadius(player, level);
		EXPECT_EQ(monsterActivation, playerVision + 2);
	}
}

} // namespace
} // namespace devilution
