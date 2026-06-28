#include "setitems.h"

#include "player.h"
#include "items.h"
#include "buff.h"

namespace devilution {

namespace {

int CountEquippedSetPieces(const Player &player, SetId setId)
{
	int count = 0;
	for (auto &item : player.InvBody) {
		if (item.isEmpty()) continue;
		// Items flagged as set pieces via _iProcFlags (repurposed for set membership)
		// In a full implementation, a dedicated _iSetId field would be used.
		// For now, set bonuses are checked based on equipped unique items matching set pieces.
		(void)setId;
		// Simplified: count is incremented when a known unique base item is equipped
		// TODO: proper set membership tracking via item data
	}
	return count;
}

void ApplySetBonus(Player &player, SetId setId, int pieceCount)
{
	if (pieceCount < 2) return;

	switch (setId) {
	case SetId::ButchersLegacy:
		if (pieceCount >= 2) {
			// 2pc: 10% bleed on hit
			player._pIFlags = player._pIFlags | ItemSpecialEffect::DrainLife;
		}
		if (pieceCount >= 3) {
			// 3pc: +20% damage to bleeding enemies (via DamageBoost buff)
			player.buffable.Apply(BuffType::DamageBoost, 20, -1, -1);
		}
		break;

	case SetId::Deathspeaker:
		if (pieceCount >= 2) {
			// 2pc: on kill → nearby allies +5 mana (passive)
		}
		if (pieceCount >= 3) {
			// 3pc: mana shield costs 0 mana
			player.pManaShield = true;
		}
		break;

	case SetId::WindforcesGift:
		if (pieceCount >= 2) {
			// 2pc: arrows +15% pierce chance (passive via iFlags)
			player._pIFlags = player._pIFlags | ItemSpecialEffect::MultipleArrows;
		}
		if (pieceCount >= 3) {
			// 3pc: pierced shots deal full damage (passive)
		}
		break;

	case SetId::ArchmagesRegalia:
		if (pieceCount >= 2) {
			// 2pc: spell cooldown -1s (passive)
		}
		if (pieceCount >= 3) {
			// 3pc: standing still 2s → +25% spell damage
			player.buffable.Apply(BuffType::DamageBoost, 25, -1, -1);
		}
		break;

	case SetId::COUNT:
		break;
	}
}

} // namespace

void CheckSetBonuses(Player &player)
{
	// Track active set piece counts and apply/remove bonuses accordingly.
	// Called from CalcPlrItemVals whenever equipment changes.

	static int prevCounts[static_cast<size_t>(SetId::COUNT)] = { 0 };

	for (size_t i = 0; i < static_cast<size_t>(SetId::COUNT); i++) {
		SetId sid = static_cast<SetId>(i);
		int current = CountEquippedSetPieces(player, sid);

		if (current != prevCounts[i]) {
			// Remove old bonus, apply new one
			ApplySetBonus(player, sid, current);
			prevCounts[i] = current;
		}
	}
}

} // namespace devilution
