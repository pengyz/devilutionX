/**
 * @file player_test.h
 *
 * Helpers for player related tests.
 */
#pragma once

#include "items.h"
#include "player.h"

using namespace devilution;

static size_t CountItems(devilution::Item *items, int n)
{
	return std::count_if(items, items + n, [](devilution::Item x) { return !x.isEmpty(); });
}

/**
 * @brief Counts items including stack counts, so the result does not depend on
 *        whether identical consumables share a slot.
 *
 * Use this when asserting "how many of a thing the player has" rather than "how
 * many slots are occupied". The two diverge once consumables stack: the rogue's
 * two starting healing potions occupy one belt slot with a stack count of two.
 */
static size_t CountItemsWithStacks(devilution::Item *items, int n)
{
	size_t total = 0;
	for (int i = 0; i < n; i++) {
		if (items[i].isEmpty())
			continue;
		total += std::max<int>(1, items[i]._iStackCount);
	}
	return total;
}

static size_t Count8(int8_t *ints, int n)
{
	return std::count_if(ints, ints + n, [](int8_t x) { return x != 0; });
}

static size_t CountU8(uint8_t *ints, int n)
{
	return std::count_if(ints, ints + n, [](uint8_t x) { return x != 0; });
}

static size_t CountBool(bool *bools, int n)
{
	return std::count_if(bools, bools + n, [](bool x) { return x; });
}
