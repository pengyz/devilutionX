#include <gtest/gtest.h>

#include "lighting.h"
#include "player.h"

namespace devilution {
namespace {

class LightSuppressionTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
	}
};

TEST_F(LightSuppressionTest, CathedralNoSuppression)
{
	// Cathedral (levels 1-4) should have no suppression
	float multiplier = GetLightSuppressionMultiplier(1);
	EXPECT_FLOAT_EQ(multiplier, 1.0f);

	multiplier = GetLightSuppressionMultiplier(4);
	EXPECT_FLOAT_EQ(multiplier, 1.0f);
}

TEST_F(LightSuppressionTest, CatacombsSlightSuppression)
{
	// Catacombs (levels 5-8) should have 90% suppression
	float multiplier = GetLightSuppressionMultiplier(5);
	EXPECT_FLOAT_EQ(multiplier, 0.9f);

	multiplier = GetLightSuppressionMultiplier(8);
	EXPECT_FLOAT_EQ(multiplier, 0.9f);
}

TEST_F(LightSuppressionTest, CavesSuppression)
{
	// Caves (levels 9-12) should have 80% suppression
	float multiplier = GetLightSuppressionMultiplier(9);
	EXPECT_FLOAT_EQ(multiplier, 0.8f);

	multiplier = GetLightSuppressionMultiplier(12);
	EXPECT_FLOAT_EQ(multiplier, 0.8f);
}

TEST_F(LightSuppressionTest, HellSuppression)
{
	// Hell (levels 13-16) should have 60% suppression
	float multiplier = GetLightSuppressionMultiplier(13);
	EXPECT_FLOAT_EQ(multiplier, 0.6f);

	multiplier = GetLightSuppressionMultiplier(16);
	EXPECT_FLOAT_EQ(multiplier, 0.6f);
}

TEST_F(LightSuppressionTest, CryptSuppression)
{
	// Crypt (levels 21-24) should have 50% suppression
	float multiplier = GetLightSuppressionMultiplier(21);
	EXPECT_FLOAT_EQ(multiplier, 0.5f);

	multiplier = GetLightSuppressionMultiplier(24);
	EXPECT_FLOAT_EQ(multiplier, 0.5f);
}

TEST_F(LightSuppressionTest, SuppressionAffectsPlayerLightRadius)
{
	// Set up player with base light radius 10
	Player &player = Players[0];
	player._pLightRad = 10;

	// In caves (80% suppression), effective radius should be 8
	int effectiveRadius = GetEffectiveLightRadius(player, 9);
	EXPECT_EQ(effectiveRadius, 8);
}

TEST_F(LightSuppressionTest, EquipmentBonusNotSuppressed)
{
	// Player with light equipment
	Player &player = Players[0];
	player._pLightRad = 12; // Base 10 + 2 from equipment

	// In caves (80% suppression), effective radius should be 10 (8 + 2)
	int effectiveRadius = GetEffectiveLightRadius(player, 9);
	EXPECT_EQ(effectiveRadius, 10);
}

TEST_F(LightSuppressionTest, NoSuppressionInTown)
{
	// Town (level 0) should have no suppression
	float multiplier = GetLightSuppressionMultiplier(0);
	EXPECT_FLOAT_EQ(multiplier, 1.0f);
}

} // namespace
} // namespace devilution
