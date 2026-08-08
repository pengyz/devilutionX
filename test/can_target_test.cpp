#include <gtest/gtest.h>

#include <algorithm>

#include "levels/gendung.h"
#include "options.h"
#include "player.h"

namespace devilution {
namespace {

class CanTargetTest : public ::testing::Test {
public:
	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
		MyPlayer->_pInfraFlag = false;
		// Default test state: neither Lit nor Visible.
		std::fill(&dFlags[0][0], &dFlags[0][0] + MAXDUNX * MAXDUNY, DungeonFlag::None);
		GetOptions().Gameplay.darkExpedition.SetValue(false);
	}

	Point p { 10, 10 };

	void SetLit() { dFlags[p.x][p.y] |= DungeonFlag::Lit; }
	void SetVisible() { dFlags[p.x][p.y] |= DungeonFlag::Visible; }
};

TEST_F(CanTargetTest, LitTileAlwaysTargetable)
{
	SetLit();
	EXPECT_TRUE(CanTarget(p));
}

TEST_F(CanTargetTest, UnlitInvisibleTileNeverTargetable)
{
	EXPECT_FALSE(CanTarget(p));
}

TEST_F(CanTargetTest, VisibleButUnlitRequiresSwitchAndInfra)
{
	SetVisible();
	// Switch off, infra on: not targetable (switch gates the whole feature).
	MyPlayer->_pInfraFlag = true;
	EXPECT_FALSE(CanTarget(p));

	// Switch on, infra off: not targetable.
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	MyPlayer->_pInfraFlag = false;
	EXPECT_FALSE(CanTarget(p));

	// Switch on, infra on: targetable.
	MyPlayer->_pInfraFlag = true;
	EXPECT_TRUE(CanTarget(p));
}

TEST_F(CanTargetTest, NoMyPlayerIsSafe)
{
	SetVisible();
	MyPlayer = nullptr;
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	EXPECT_FALSE(CanTarget(p));
}

} // namespace
} // namespace devilution
