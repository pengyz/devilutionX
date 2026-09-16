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
#include "utils/log.hpp"
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

} // namespace

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

namespace {

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

uint8_t BehaviorClassCapForLevel(uint8_t level, BehaviorClass cls)
{
	// Mirrors monster.cpp's B1 scatter-sampling cap (see :3513-3531): L9-12 caps the
	// RangedKite class at 2, L13-16 caps any single class at 2, everything else is uncapped.
	// The validator and the sampling loop share this single source of truth so they cannot drift.
	if (level >= 13 && level <= 16)
		return 2;
	if (level >= 9 && level <= 12 && cls == BehaviorClass::RangedKite)
		return 2;
	return 0;
}

void SortRosterByLevel(std::span<LevelRosterEntry> entries)
{
	std::stable_sort(entries.begin(), entries.end(), [](const LevelRosterEntry &a, const LevelRosterEntry &b) {
		return a.level < b.level;
	});
}

std::span<const LevelRosterEntry> FindLevelRoster(std::span<const LevelRosterEntry> entries, uint8_t level)
{
	const auto begin = std::find_if(entries.begin(), entries.end(), [level](const LevelRosterEntry &e) { return e.level == level; });
	if (begin == entries.end())
		return {};
	const auto end = std::find_if(begin, entries.end(), [level](const LevelRosterEntry &e) { return e.level != level; });
	return { &*begin, static_cast<size_t>(std::distance(begin, end)) };
}

std::optional<std::string> ValidateLevelRoster(std::span<const LevelRosterEntry> entries, std::span<const LevelRosterParams> params)
{
	// Type existence/bounds is shared by both modes: a row naming an id outside
	// MonstersData's range (e.g. a smuggled MT_INVALID) must be rejected before either
	// mode's relaxed or full checks run, otherwise downstream code could index out of bounds.
	for (const LevelRosterEntry &entry : entries) {
		if (static_cast<size_t>(entry.type) >= MonstersData.size())
			return StrCat("roster row names unknown monster id ", static_cast<int>(entry.type));
	}

	// Range/shape sanity checks apply in both modes; they are basic structural
	// requirements, not retail-specific availability/unique semantics.
	for (const LevelRosterParams &param : params) {
		if (param.maxImage <= 0)
			return StrCat("level ", param.level, " has a non-positive max_image (", param.maxImage, ")");
		if (param.tailDraw < 0)
			return StrCat("level ", param.level, " has a negative tail_draw (", param.tailDraw, ")");
		for (const auto &[cls, floor] : param.classFloors) {
			if (cls == BehaviorClass::Count)
				return StrCat("level ", param.level, " class floor names the sentinel BehaviorClass::Count, which is not a real category");
		}
	}

	// Uniqueness checks (R21 + 2a rulings): a duplicate params row for the same level would
	// have its second occurrence silently ignored by GetLevelRosterParams() (linear
	// find_if returns the first match); a duplicate (level, monster_id) entry row would
	// silently double-count that monster in caps/floor bookkeeping. Both apply in both modes
	// since they are structural, not availability-related.
	{
		std::vector<uint8_t> paramLevels;
		for (const LevelRosterParams &param : params)
			paramLevels.push_back(param.level);
		std::sort(paramLevels.begin(), paramLevels.end());
		const auto dup = std::adjacent_find(paramLevels.begin(), paramLevels.end());
		if (dup != paramLevels.end())
			return StrCat("level ", *dup, " has more than one params row");
	}
	{
		std::vector<std::pair<uint8_t, _monster_id>> entryKeys;
		for (const LevelRosterEntry &entry : entries)
			entryKeys.emplace_back(entry.level, entry.type);
		std::sort(entryKeys.begin(), entryKeys.end());
		const auto dup = std::adjacent_find(entryKeys.begin(), entryKeys.end());
		if (dup != entryKeys.end())
			return StrCat("level ", dup->first, " has more than one roster row for monster ", static_cast<int>(dup->second));
	}

	// Core-non-empty check: every distinct level appearing in entries or params must have
	// at least one core member. This is enforced in both modes.
	std::vector<uint8_t> levels;
	for (const LevelRosterEntry &entry : entries)
		levels.push_back(entry.level);
	for (const LevelRosterParams &param : params)
		levels.push_back(param.level);
	std::sort(levels.begin(), levels.end());
	levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
	for (const uint8_t level : levels) {
		const bool hasCore = std::any_of(entries.begin(), entries.end(), [level](const LevelRosterEntry &e) {
			return e.level == level && e.role == LevelRosterRole::Core;
		});
		if (!hasCore)
			return StrCat("level ", level, " has no core roster members");
	}

	// Core-vs-cap self-consistency (R29). The sampling loop applies the B1 caps to the
	// tail draw only: the core roster is pre-added ahead of the loop and therefore bypasses
	// them by design (spec 4.2.2). That makes it possible for the DATA to break a B1
	// guarantee that the code can no longer defend - e.g. a level whose core lists three
	// RangedKite monsters is an all-kite level no matter what the loop does afterwards.
	// Reject that here, at load time, so the guarantee is structural rather than something
	// a percentage baseline happens to notice. Checked in both modes: it is a property of
	// the table itself, and the availability filter the pre-add applies can only ever make
	// the realised core a SUBSET of these rows, so passing here is the conservative bound.
	for (const uint8_t level : levels) {
		uint8_t coreClassCounts[static_cast<size_t>(BehaviorClass::Count)] = {};
		for (const LevelRosterEntry &entry : entries) {
			if (entry.level != level || entry.role != LevelRosterRole::Core)
				continue;
			coreClassCounts[static_cast<size_t>(GetBehaviorClass(MonstersData[static_cast<size_t>(entry.type)].ai))]++;
		}
		for (size_t i = 0; i < static_cast<size_t>(BehaviorClass::Count); i++) {
			const auto cls = static_cast<BehaviorClass>(i);
			const uint8_t cap = BehaviorClassCapForLevel(level, cls);
			if (cap == 0)
				continue;
			if (coreClassCounts[i] > cap)
				return StrCat("level ", level, " core roster has ", static_cast<int>(coreClassCounts[i]),
				    " monsters of behaviour class ", magic_enum::enum_name(cls),
				    " but the B1 sampling cap for that level and class is ", static_cast<int>(cap),
				    "; core bypasses the cap, so this breaks the guarantee in the data");
		}
	}

	if (gbIsSpawn) {
		// Shareware (spawn) data ships a much smaller monster/unique set, so the full
		// retail-style checks (per-row availability, unique-base whitelist, class floor
		// satisfiability) are not meaningful here and are relaxed. Type existence and the
		// core-non-empty check above are still shared with retail mode.
		return std::nullopt;
	}

	for (const LevelRosterEntry &entry : entries) {
		if (!IsAvailableAt(entry.level, entry.type))
			return StrCat("monster ", static_cast<int>(entry.type), " is not available at level ", entry.level);
		if (!entry.allowUniqueBoost && IsUniqueBaseForLevel(entry.level, entry.type))
			return StrCat("monster ", static_cast<int>(entry.type), " is a unique's base at level ", entry.level, " and needs allow_unique_boost");
	}
	for (const LevelRosterParams &param : params) {
		// Class floor satisfiability: the level's available candidate pool, after being
		// truncated by the B1 sampling cap, must still contain at least `floor` monsters
		// of that behavior class.
		for (const auto &[cls, floor] : param.classFloors) {
			size_t available = 0;
			for (size_t i = 0; i < MonstersData.size(); i++) {
				const auto type = static_cast<_monster_id>(i);
				if (IsAvailableAt(param.level, type) && GetBehaviorClass(MonstersData[i].ai) == cls)
					available++;
			}
			const uint8_t cap = BehaviorClassCapForLevel(param.level, cls);
			const size_t effective = cap == 0 ? available : std::min(available, static_cast<size_t>(cap));
			if (effective < floor)
				return StrCat("level ", param.level, " class floor ", static_cast<int>(cls), " needs ", floor, " but only ", effective, " candidates exist (", available, " raw candidates, B1 sampling caps this class to ", static_cast<int>(cap), ")");
		}
	}
	return std::nullopt;
}

namespace {

// Verbose-only load-time diagnostic: reports each Phase A level's roster size and params so a
// missing/short level shows up in a verbose log instead of only surfacing later as a silent
// empty span at GetLevelRoster() call sites. This is the production caller for
// GetLevelRoster()/GetLevelRosterParams() (Task 3's sampling loop is its own, separate caller,
// out of this task's scope); it does not affect gameplay.
void LogLoadedRosterSummary()
{
	// Iterate the levels actually present in Params (rather than a hardcoded Phase A range like
	// 1..16) so this diagnostic keeps covering every loaded level once Phase A2 adds the L17-24
	// HF overlay rows, with no edit required here.
	for (const LevelRosterParams &param : Params) {
		const std::span<const LevelRosterEntry> roster = GetLevelRoster(param.level);
		const LevelRosterParams *params = GetLevelRosterParams(param.level);
		LogVerbose("Level roster: level {} has {} entries, params {}", param.level, roster.size(),
		    params != nullptr ? "present" : "missing");
	}
}

} // namespace

void LoadLevelRosterFromFiles(std::string_view rosterFile, std::string_view paramsFile)
{
	Entries.clear();
	Params.clear();

	DataFile rosterDataFile = DataFile::loadOrDie(rosterFile);
	LoadLevelRosterEntriesFromFile(rosterDataFile, rosterFile);

	DataFile paramsDataFile = DataFile::loadOrDie(paramsFile);
	LoadLevelRosterParamsFromFile(paramsDataFile, paramsFile);

	// GetLevelRoster() relies on same-level rows being physically contiguous. Nothing
	// guarantees the TSV rows are grouped by level, so sort them here (stably, so rows
	// sharing a level keep their file order, which matters for tail draw ordering).
	SortRosterByLevel(Entries);

	Entries.shrink_to_fit();
	Params.shrink_to_fit();

	const std::optional<std::string> error = ValidateLevelRoster(Entries, Params);
	if (error.has_value())
		app_fatal(StrCat("Level roster validation failed: ", *error));

	LogLoadedRosterSummary();
}

void LoadLevelRoster()
{
	LoadLevelRosterFromFiles("txtdata\\monsters\\level_rosters.tsv", "txtdata\\monsters\\level_roster_params.tsv");
}

std::span<const LevelRosterEntry> GetLevelRoster(uint8_t level)
{
	return FindLevelRoster(Entries, level);
}

const LevelRosterParams *GetLevelRosterParams(uint8_t level)
{
	const auto it = std::find_if(Params.begin(), Params.end(), [level](const LevelRosterParams &p) { return p.level == level; });
	return it == Params.end() ? nullptr : &*it;
}

} // namespace devilution
