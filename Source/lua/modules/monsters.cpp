#include "lua/modules/monsters.hpp"

#include <string_view>

#include <fmt/format.h>
#include <sol/sol.hpp>

#include "data/file.hpp"
#include "engine/point.hpp"
#include "lua/metadoc.hpp"
#include "monster.h"
#include "tables/monstdat.h"
#include "utils/language.h"
#include "utils/str_split.hpp"

namespace devilution {

namespace {

void AddMonsterDataFromTsv(const std::string_view path)
{
	DataFile dataFile = DataFile::loadOrDie(path);
	LoadMonstDatFromFile(dataFile, path, true);
}

void AddUniqueMonsterDataFromTsv(const std::string_view path)
{
	DataFile dataFile = DataFile::loadOrDie(path);
	LoadUniqueMonstDatFromFile(dataFile, path);
}

int64_t CreateMonsterLua(std::string_view monsterName, int x, int y)
{
	for (size_t i = 0; i < MonstersData.size(); i++) {
		if (MonstersData[i].name == monsterName) {
			if (ActiveMonsterCount >= MaxMonsters)
				return -1;
			const Point pos = { x, y };
			Monster *monster = AddMonster(pos, Direction::South, i, true);
			if (monster == nullptr)
				return -1;
			return reinterpret_cast<int64_t>(monster);
		}
	}
	return -1;
}

void SetMonsterAILua(int64_t monsterId, int aiType)
{
	Monster *monster = reinterpret_cast<Monster *>(static_cast<uintptr_t>(monsterId));
	if (monster != nullptr && !monster->isInvalid) {
		monster->ai = static_cast<MonsterAIID>(static_cast<int8_t>(aiType));
	}
}

void InitMonsterUserType(sol::state_view &lua)
{
	sol::usertype<Monster> monsterType = lua.new_usertype<Monster>(sol::no_constructor);
	LuaSetDocReadonlyProperty(monsterType, "position", "Point",
	    "Monster's current position",
	    [](const Monster &monster) {
		    return Point { monster.position.tile };
	    });
	LuaSetDocReadonlyProperty(monsterType, "id", "integer",
	    "Monster's unique ID",
	    [](const Monster &monster) {
		    return reinterpret_cast<int64_t>(&monster);
	    });
	LuaSetDocProperty(monsterType, "ai", "integer",
	    "Monster's AI type",
	    [](const Monster &m) { return static_cast<int>(static_cast<int8_t>(m.ai)); },
	    [](Monster &m, int v) { m.ai = static_cast<MonsterAIID>(static_cast<int8_t>(v)); });
	LuaSetDocProperty(monsterType, "hitPoints", "integer",
	    "Current hit points",
	    [](const Monster &m) { return m.hitPoints; },
	    [](Monster &m, int v) { m.hitPoints = v; });
}

} // namespace

sol::table LuaMonstersModule(sol::state_view &lua)
{
	InitMonsterUserType(lua);
	sol::table table = lua.create_table();
	LuaSetDocFn(table, "addMonsterDataFromTsv", "(path: string)", AddMonsterDataFromTsv);
	LuaSetDocFn(table, "addUniqueMonsterDataFromTsv", "(path: string)", AddUniqueMonsterDataFromTsv);
	LuaSetDocFn(table, "CreateMonster", "(name: string, x: number, y: number) -> id", CreateMonsterLua);
	LuaSetDocFn(table, "SetAI", "(monsterId: number, aiType: number)", SetMonsterAILua);
	return table;
}

} // namespace devilution
