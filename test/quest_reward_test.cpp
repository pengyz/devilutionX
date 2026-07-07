#include <gtest/gtest.h>

#include "quests.h"
#include "stores.h"
#include "tables/objdat.h"

namespace devilution {
namespace {

class QuestRewardTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		// Initialize quest state for testing
		for (auto &quest : Quests) {
			quest._qactive = QUEST_NOTAVAIL;
		}
	}
};

TEST_F(QuestRewardTest, AdriaOffersSpellChoiceAfterMushroomQuest)
{
	// Set mushroom quest as done
	Quests[Q_MUSHROOM]._qactive = QUEST_DONE;

	// Check that Adria offers the choice
	bool offersChoice = DoesAdriaOfferSpellChoice();
	EXPECT_TRUE(offersChoice);
}

TEST_F(QuestRewardTest, AdriaDoesNotOfferSpellChoiceBeforeMushroomQuest)
{
	// Mushroom quest is not done
	Quests[Q_MUSHROOM]._qactive = QUEST_NOTAVAIL;

	// Check that Adria does not offer the choice
	bool offersChoice = DoesAdriaOfferSpellChoice();
	EXPECT_FALSE(offersChoice);
}

TEST_F(QuestRewardTest, PepinGivesRegenerationPotionAfterPoisonWaterQuest)
{
	// Set poison water quest as done
	Quests[Q_PWATER]._qactive = QUEST_DONE;

	// Check that Pepin gives regeneration potion
	bool givesPotion = DoesPepinGiveRegenerationPotion();
	EXPECT_TRUE(givesPotion);
}

TEST_F(QuestRewardTest, PepinDoesNotGiveRegenerationPotionBeforePoisonWaterQuest)
{
	// Poison water quest is not done
	Quests[Q_PWATER]._qactive = QUEST_NOTAVAIL;

	// Check that Pepin does not give regeneration potion
	bool givesPotion = DoesPepinGiveRegenerationPotion();
	EXPECT_FALSE(givesPotion);
}

TEST_F(QuestRewardTest, GriswoldOffersCustomWeaponAfterAnvilQuest)
{
	// Set anvil quest as done
	Quests[Q_ANVIL]._qactive = QUEST_DONE;

	// Check that Griswold offers custom weapon
	bool offersWeapon = DoesGriswoldOfferCustomWeapon();
	EXPECT_TRUE(offersWeapon);
}

TEST_F(QuestRewardTest, GriswoldDoesNotOfferCustomWeaponBeforeAnvilQuest)
{
	// Anvil quest is not done
	Quests[Q_ANVIL]._qactive = QUEST_NOTAVAIL;

	// Check that Griswold does not offer custom weapon
	bool offersWeapon = DoesGriswoldOfferCustomWeapon();
	EXPECT_FALSE(offersWeapon);
}

} // namespace
} // namespace devilution
