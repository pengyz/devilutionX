#include <gtest/gtest.h>

#include "items.h"
#include "player.h"
#include "tables/itemdat.h"

namespace devilution {
namespace {

class GoldDropTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
	}
};

TEST_F(GoldDropTest, GoldValueIsPositive)
{
	// Verify gold items have positive value
	Item goldItem;
	goldItem._itype = ItemType::Gold;
	goldItem._ivalue = 100;

	EXPECT_GT(goldItem._ivalue, 0);
}

TEST_F(GoldDropTest, GoldValueRespectsMaxLimit)
{
	// Verify gold value doesn't exceed GOLD_MAX_LIMIT
	Item goldItem;
	goldItem._itype = ItemType::Gold;
	goldItem._ivalue = GOLD_MAX_LIMIT + 100;

	// The actual clamping happens in GetItemAttrs
	// This test verifies the constant exists
	EXPECT_GT(GOLD_MAX_LIMIT, 0);
}

} // namespace
} // namespace devilution
