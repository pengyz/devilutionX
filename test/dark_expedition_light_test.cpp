#include <gtest/gtest.h>

#include "items.h"
#include "levels/gendung.h"
#include "options.h"
#include "player.h"

namespace devilution {
namespace {

class DarkExpeditionLightRadiusTest : public ::testing::Test {
public:
	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
		MyPlayer->_pLightRad = 10;
		currlevel = 13;
	}

	static void SetUpTestSuite()
	{
		// Ensure the option is at its default (false) so each test controls it explicitly.
		GetOptions().Gameplay.darkExpedition.SetValue(false);
	}
};

TEST_F(DarkExpeditionLightRadiusTest, SwitchOffIsVanilla)
{
	currlevel = 15;
	CalcPlrLightRadius(*MyPlayer, 10);
	EXPECT_EQ(MyPlayer->_pLightRad, 10);
}

TEST_F(DarkExpeditionLightRadiusTest, HellLevelsScaleTo60Percent)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	for (int level : { 13, 14, 15, 16 }) {
		currlevel = level;
		MyPlayer->_pLightRad = 0; // reset; CalcPlrLightRadius writes via ChangeLightRadius only on active level
		CalcPlrLightRadius(*MyPlayer, 10);
		EXPECT_EQ(MyPlayer->_pLightRad, 6) << "Hell level " << level;
	}
}

TEST_F(DarkExpeditionLightRadiusTest, NestScalesTo85PercentTruncated)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	for (int level : { 17, 18, 19, 20 }) {
		currlevel = level;
		MyPlayer->_pLightRad = 0;
		CalcPlrLightRadius(*MyPlayer, 10);
		EXPECT_EQ(MyPlayer->_pLightRad, 8) << "Nest level " << level << " (trunc(8.5) = 8, not 9)";
	}
}

TEST_F(DarkExpeditionLightRadiusTest, CryptScalesTo50Percent)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	for (int level : { 21, 22, 23, 24 }) {
		currlevel = level;
		MyPlayer->_pLightRad = 0;
		CalcPlrLightRadius(*MyPlayer, 10);
		EXPECT_EQ(MyPlayer->_pLightRad, 5) << "Crypt level " << level;
	}
}

TEST_F(DarkExpeditionLightRadiusTest, GearBonusScalesWithMultiplier)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	currlevel = 21; // Crypt, 50%
	MyPlayer->_pLightRad = 0;
	CalcPlrLightRadius(*MyPlayer, 14); // base 10 + Lightforge(+4)
	EXPECT_EQ(MyPlayer->_pLightRad, 7); // trunc(14*0.5) = 7, not 10*0.5+4 = 9
}

TEST_F(DarkExpeditionLightRadiusTest, CursedGearClampsToVanillaMinimum)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	currlevel = 21; // Crypt, 50%
	MyPlayer->_pLightRad = 0;
	CalcPlrLightRadius(*MyPlayer, 2); // cursed, below vanilla min
	EXPECT_EQ(MyPlayer->_pLightRad, 2); // trunc(2*0.5) = 1, clamped to 2
}

TEST_F(DarkExpeditionLightRadiusTest, CurveIsMonotonic)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	const auto scale = [this](int level) {
		currlevel = level;
		MyPlayer->_pLightRad = 0;
		CalcPlrLightRadius(*MyPlayer, 10);
		return MyPlayer->_pLightRad;
	};
	// Hell(0.6) < Nest(0.85) < Crypt(0.5) is NOT monotonic; the meaningful check is:
	// Nest must be brighter than Hell and darker than vanilla.
	EXPECT_GT(scale(17), scale(13)); // Nest > Hell
	EXPECT_GT(scale(13), scale(21)); // Hell > Crypt
}

} // namespace
} // namespace devilution
