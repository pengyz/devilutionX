#include "lua/modules/world.hpp"

#include <sol/sol.hpp>

#include "lua/metadoc.hpp"
#include "missiles.h"
#include "monster.h"
#include "player.h"
#include "tables/misdat.h"
#include "tables/spelldat.h"

namespace devilution {

namespace {

int64_t SpawnMissile(std::string_view missileName, int x, int y, int direction)
{
	const auto missileId = ParseSpellId(missileName);
	if (!missileId.has_value()) {
		return -1;
	}

	const SpellID spellId = *missileId;
	const MissileID mId = GetSpellData(spellId).sMissiles[0];

	if (mId == MissileID::Null)
		return -1;

	if (Players.empty() || MyPlayer == nullptr)
		return -1;

	const Point src = { x, y };
	const Point dst = { x + 1, y };
	const Direction dir = static_cast<Direction>(direction % 8);

	Missile *missile = AddMissile(src, dst, dir, mId, TARGET_MONSTERS, *MyPlayer, 0, 0);
	if (missile == nullptr)
		return -1;

	return reinterpret_cast<int64_t>(missile);
}

void DamageTarget(int64_t targetId, int damage, std::string_view damageType)
{
	if (damage <= 0)
		return;

	DamageType dType = DamageType::Physical;
	if (damageType == "Fire") dType = DamageType::Fire;
	else if (damageType == "Lightning") dType = DamageType::Lightning;
	else if (damageType == "Magic") dType = DamageType::Magic;
	else if (damageType == "Acid") dType = DamageType::Acid;

	Monster *monster = reinterpret_cast<Monster *>(static_cast<uintptr_t>(targetId));
	if (monster != nullptr && !monster->isInvalid && monster->hitPoints > 0) {
		ApplyMonsterDamage(dType, *monster, damage);
		return;
	}

	Player *player = reinterpret_cast<Player *>(static_cast<uintptr_t>(targetId));
	if (player != nullptr && player->plractive && player->_pHitPoints > 0) {
		player->_pHitPoints = std::max(0, player->_pHitPoints - (damage << 6));
	}
}

sol::table GetMonstersInRange(int x, int y, int radius, sol::this_state L)
{
	sol::state_view lua = L;
	sol::table result = lua.create_table();
	int index = 1;

	const WorldTilePosition center = { static_cast<WorldTileCoord>(x), static_cast<WorldTileCoord>(y) };

	for (size_t i = 0; i < ActiveMonsterCount; i++) {
		const Monster &m = Monsters[ActiveMonsters[i]];
		if (m.isInvalid || m.hitPoints <= 0)
			continue;
		if (m.position.tile.WalkingDistance(center) <= radius) {
			result[index++] = reinterpret_cast<int64_t>(&m);
		}
	}

	return result;
}

}

sol::table LuaWorldModule(sol::state_view &lua)
{
	sol::table table = lua.create_table();
	LuaSetDocFn(table, "SpawnMissile", "(missileName, x, y, direction) -> id",
	    SpawnMissile);
	LuaSetDocFn(table, "DamageTarget", "(targetId, damage, damageType)",
	    DamageTarget);
	LuaSetDocFn(table, "GetMonstersInRange", "(x, y, radius) -> table",
	    GetMonstersInRange);
	return table;
}

}
