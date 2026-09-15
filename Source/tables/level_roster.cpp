/**
 * @file level_roster.cpp
 *
 * Implementation of the per-level monster roster tables (types, loader, validation).
 */
#include "tables/level_roster.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <magic_enum/magic_enum.hpp>

#include "appfat.h"
#include "data/file.hpp"
#include "data/record_reader.hpp"
#include "game_mode.hpp"
#include "tables/monstdat.h"
#include "utils/str_cat.hpp"
#include "utils/str_split.hpp"

namespace devilution {

namespace {

std::vector<LevelRosterEntry> Entries;
std::vector<LevelRosterParams> Params;

// Same semantics as monster.cpp's IsMonsterAvailable (that function has internal
// linkage there, so this validator re-implements the check rather than reusing it).
bool IsAvailableAt(uint8_t level, _monster_id type)
{
	const MonsterData &data = MonstersData[static_cast<size_t>(type)];
	if (data.availability == MonsterAvailability::Never)
		return false;
	if (gbIsSpawn && data.availability == MonsterAvailability::Retail)
		return false;
	return level >= data.minDunLvl && level <= data.maxDunLvl;
}

bool IsUniqueBaseForLevel(uint8_t level, _monster_id type)
{
	for (const UniqueMonsterData &unique : UniqueMonstersData) {
		if (unique.mlevel == level && unique.mtype == type)
			return true;
	}
	return false;
}

std::vector<std::pair<BehaviorClass, uint8_t>> ParseClassFloors(std::string_view value)
{
	std::vector<std::pair<BehaviorClass, uint8_t>> result;
	if (value.empty())
		return result;
	for (const std::string_view part : SplitByChar(value, ',')) {
		const size_t eq = part.find('=');
		if (eq == std::string_view::npos)
			app_fatal(StrCat("Invalid class_floors entry (missing '='): \"", part, "\""));
		const std::string_view clsName = part.substr(0, eq);
		const std::string_view floorStr = part.substr(eq + 1);
		const std::optional<BehaviorClass> cls = magic_enum::enum_cast<BehaviorClass>(clsName);
		if (!cls.has_value())
			app_fatal(StrCat("Invalid class_floors entry (unknown BehaviorClass): \"", clsName, "\""));
		uint8_t floor = 0;
		const std::from_chars_result parsed = std::from_chars(floorStr.data(), floorStr.data() + floorStr.size(), floor);
		if (parsed.ec != std::errc() || parsed.ptr != floorStr.data() + floorStr.size())
			app_fatal(StrCat("Invalid class_floors entry (bad integer): \"", floorStr, "\""));
		result.emplace_back(*cls, floor);
	}
	return result;
}

void LoadLevelRosterEntriesFromFile(DataFile &dataFile, std::string_view filename)
{
	dataFile.skipHeaderOrDie(filename);
	Entries.reserve(Entries.size() + dataFile.numRecords());
	for (DataFileRecord record : dataFile) {
		RecordReader reader { record, filename };
		LevelRosterEntry entry {};
		reader.readInt("level", entry.level);
		reader.read("monster_id", entry.type, [](std::string_view value) -> std::expected<_monster_id, std::string> {
			const std::optional<_monster_id> id = magic_enum::enum_cast<_monster_id>(value);
			if (!id.has_value())
				return std::unexpected(StrCat("Unknown monster id \"", value, "\""));
			return *id;
		});
		reader.read("role", entry.role, [](std::string_view value) -> std::expected<LevelRosterRole, std::string> {
			if (value == "core")
				return LevelRosterRole::Core;
			if (value == "tail")
				return LevelRosterRole::Tail;
			return std::unexpected(StrCat("Unknown role \"", value, "\""));
		});
		reader.read("allow_unique_boost", entry.allowUniqueBoost, [](std::string_view value) -> std::expected<bool, std::string> {
			if (value == "-" || value.empty())
				return false;
			if (value == "true")
				return true;
			if (value == "false")
				return false;
			return std::unexpected(StrCat("Invalid allow_unique_boost value \"", value, "\""));
		});
		Entries.push_back(entry);
	}
}

void LoadLevelRosterParamsFromFile(DataFile &dataFile, std::string_view filename)
{
	dataFile.skipHeaderOrDie(filename);
	Params.reserve(Params.size() + dataFile.numRecords());
	for (DataFileRecord record : dataFile) {
		RecordReader reader { record, filename };
		LevelRosterParams params {};
		reader.readInt("level", params.level);
		reader.readInt("max_image", params.maxImage);
		reader.readInt("tail_draw", params.tailDraw);
		std::string classFloorsStr;
		reader.readString("class_floors", classFloorsStr);
		params.classFloors = ParseClassFloors(classFloorsStr);
		Params.push_back(params);
	}
}

} // namespace

std::optional<std::string> ValidateLevelRoster(std::span<const LevelRosterEntry> entries, std::span<const LevelRosterParams> params)
{
	for (const LevelRosterEntry &entry : entries) {
		if (static_cast<size_t>(entry.type) >= MonstersData.size())
			return StrCat("roster row names unknown monster id ", static_cast<int>(entry.type));
		if (!IsAvailableAt(entry.level, entry.type))
			return StrCat("monster ", static_cast<int>(entry.type), " is not available at level ", entry.level);
		if (!entry.allowUniqueBoost && IsUniqueBaseForLevel(entry.level, entry.type))
			return StrCat("monster ", static_cast<int>(entry.type), " is a unique's base at level ", entry.level, " and needs allow_unique_boost");
	}
	for (const LevelRosterParams &param : params) {
		// Class floor satisfiability: the level's available candidate pool must contain at
		// least `floor` monsters of that behavior class.
		for (const auto &[cls, floor] : param.classFloors) {
			size_t available = 0;
			for (size_t i = 0; i < MonstersData.size(); i++) {
				const auto type = static_cast<_monster_id>(i);
				if (IsAvailableAt(param.level, type) && GetBehaviorClass(MonstersData[i].ai) == cls)
					available++;
			}
			if (available < floor)
				return StrCat("level ", param.level, " class floor ", static_cast<int>(cls), " needs ", floor, " but only ", available, " candidates exist");
		}
	}
	return std::nullopt;
}

void LoadLevelRoster()
{
	Entries.clear();
	Params.clear();

	const std::string_view rosterFilename = "txtdata\\monsters\\level_rosters.tsv";
	DataFile rosterFile = DataFile::loadOrDie(rosterFilename);
	LoadLevelRosterEntriesFromFile(rosterFile, rosterFilename);

	const std::string_view paramsFilename = "txtdata\\monsters\\level_roster_params.tsv";
	DataFile paramsFile = DataFile::loadOrDie(paramsFilename);
	LoadLevelRosterParamsFromFile(paramsFile, paramsFilename);

	Entries.shrink_to_fit();
	Params.shrink_to_fit();

	const std::optional<std::string> error = ValidateLevelRoster(Entries, Params);
	if (error.has_value())
		app_fatal(StrCat("Level roster validation failed: ", *error));
}

std::span<const LevelRosterEntry> GetLevelRoster(uint8_t level)
{
	const auto begin = std::find_if(Entries.begin(), Entries.end(), [level](const LevelRosterEntry &e) { return e.level == level; });
	if (begin == Entries.end())
		return {};
	const auto end = std::find_if(begin, Entries.end(), [level](const LevelRosterEntry &e) { return e.level != level; });
	return { &*begin, static_cast<size_t>(std::distance(begin, end)) };
}

const LevelRosterParams *GetLevelRosterParams(uint8_t level)
{
	const auto it = std::find_if(Params.begin(), Params.end(), [level](const LevelRosterParams &p) { return p.level == level; });
	return it == Params.end() ? nullptr : &*it;
}

} // namespace devilution
