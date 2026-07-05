#include "spell_tooltip.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <string>

#include <fmt/format.h>
#include <sol/sol.hpp>

#include "appfat.h"
#include "data/file.hpp"
#include "data/iterators.hpp"
#include "missiles.h"
#include "spells.h"
#include "utils/language.h"
#include "utils/log.hpp"

namespace devilution {

std::vector<SpellDescLine> SpellDescLines;

namespace {

sol::state &GetExprLuaState()
{
	static sol::state lua;
	return lua;
}

bool &IsExprLuaInitialized()
{
	static bool initialized = false;
	return initialized;
}

void EnsureExprLuaState()
{
	if (IsExprLuaInitialized())
		return;

	sol::state &lua = GetExprLuaState();
	lua.open_libraries(sol::lib::base, sol::lib::math);
	IsExprLuaInitialized() = true;
}

DescFormat ParseDescFormat(std::string_view value)
{
	if (value == "damage_range") return DescFormat::DamageRange;
	if (value == "heal_range") return DescFormat::HealRange;
	if (value == "value_single") return DescFormat::ValueSingle;
	if (value == "value_delta") return DescFormat::ValueDelta;
	if (value == "mana") return DescFormat::Mana;
	if (value == "mana_delta") return DescFormat::ManaDelta;
	if (value == "text") return DescFormat::Text;
	if (value == "special") return DescFormat::Special;
	if (value == "level_display") return DescFormat::LevelDisplay;
	if (value == "heal_delta") return DescFormat::HealDelta;
	app_fatal(fmt::format("Unknown DescFormat: {}", value));
	return DescFormat::Text;
}

DescSection ParseDescSection(std::string_view value)
{
	if (value == "desc") return DescSection::Desc;
	if (value == "upgrade") return DescSection::Upgrade;
	if (value == "warning") return DescSection::Warning;
	app_fatal(fmt::format("Unknown DescSection: {}", value));
	return DescSection::Desc;
}

SpellID ParseSpellIdForDesc(std::string_view value)
{
	if (value == "Null") return SpellID::Null;
	if (value == "Firebolt") return SpellID::Firebolt;
	if (value == "Healing") return SpellID::Healing;
	if (value == "Lightning") return SpellID::Lightning;
	if (value == "Flash") return SpellID::Flash;
	if (value == "Identify") return SpellID::Identify;
	if (value == "FireWall") return SpellID::FireWall;
	if (value == "TownPortal") return SpellID::TownPortal;
	if (value == "StoneCurse") return SpellID::StoneCurse;
	if (value == "Infravision") return SpellID::Infravision;
	if (value == "Phasing") return SpellID::Phasing;
	if (value == "ManaShield") return SpellID::ManaShield;
	if (value == "Fireball") return SpellID::Fireball;
	if (value == "Guardian") return SpellID::Guardian;
	if (value == "ChainLightning") return SpellID::ChainLightning;
	if (value == "FlameWave") return SpellID::FlameWave;
	if (value == "DoomSerpents") return SpellID::DoomSerpents;
	if (value == "BloodRitual") return SpellID::BloodRitual;
	if (value == "Nova") return SpellID::Nova;
	if (value == "Invisibility") return SpellID::Invisibility;
	if (value == "Inferno") return SpellID::Inferno;
	if (value == "Golem") return SpellID::Golem;
	if (value == "Rage") return SpellID::Rage;
	if (value == "Teleport") return SpellID::Teleport;
	if (value == "Apocalypse") return SpellID::Apocalypse;
	if (value == "Etherealize") return SpellID::Etherealize;
	if (value == "ItemRepair") return SpellID::ItemRepair;
	if (value == "StaffRecharge") return SpellID::StaffRecharge;
	if (value == "TrapDisarm") return SpellID::TrapDisarm;
	if (value == "Elemental") return SpellID::Elemental;
	if (value == "ChargedBolt") return SpellID::ChargedBolt;
	if (value == "HolyBolt") return SpellID::HolyBolt;
	if (value == "Resurrect") return SpellID::Resurrect;
	if (value == "Telekinesis") return SpellID::Telekinesis;
	if (value == "HealOther") return SpellID::HealOther;
	if (value == "BloodStar") return SpellID::BloodStar;
	if (value == "BoneSpirit") return SpellID::BoneSpirit;
	if (value == "Mana") return SpellID::Mana;
	if (value == "Magi") return SpellID::Magi;
	if (value == "Jester") return SpellID::Jester;
	if (value == "LightningWall") return SpellID::LightningWall;
	if (value == "Immolation") return SpellID::Immolation;
	if (value == "Warp") return SpellID::Warp;
	if (value == "Reflect") return SpellID::Reflect;
	if (value == "Berserk") return SpellID::Berserk;
	if (value == "RingOfFire") return SpellID::RingOfFire;
	if (value == "Search") return SpellID::Search;
	if (value == "RuneOfFire") return SpellID::RuneOfFire;
	if (value == "RuneOfLight") return SpellID::RuneOfLight;
	if (value == "RuneOfNova") return SpellID::RuneOfNova;
	if (value == "RuneOfImmolation") return SpellID::RuneOfImmolation;
	if (value == "RuneOfStone") return SpellID::RuneOfStone;
	app_fatal(fmt::format("Unknown SpellID: {}", value));
	return SpellID::Null;
}

} // namespace

ExprResult EvaluateSpellExpr(const std::string &expr, const Player &player, SpellID spell, int level)
{
	ExprResult result { 0, 0, 0, false };

	EnsureExprLuaState();
	sol::state &lua = GetExprLuaState();

	sol::table ctx = lua.create_table();

	ctx["lvl"] = level;
	ctx["charLevel"] = player.getCharacterLevel();
	ctx["magic"] = player._pMagic;
	ctx["mana"] = GetManaAmount(player, spell, level) >> 6;

	auto [min, max] = GetDamageAmt(spell, level);
	ctx["damage"] = lua.create_table_with("min", min, "max", max);

	const SpellData &sd = GetSpellData(spell);
	for (int i = 0; i < 8; i++)
		ctx[fmt::format("par{}", i + 1)] = sd.sParam[i];

	int lvl = level;
	ctx.set_function("ln", [lvl](int a, int b) { return a + (lvl - 1) * b; });

	sol::environment env(lua, sol::create, lua.globals());
	// Restrict to safe math functions
	env["math"] = lua.create_table_with(
	    "floor", [](double x) { return static_cast<int>(std::floor(x)); },
	    "ceil", [](double x) { return static_cast<int>(std::ceil(x)); },
	    "min", [](int a, int b) { return std::min(a, b); },
	    "max", [](int a, int b) { return std::max(a, b); });

	// Copy context vars into environment
	for (auto &[k, v] : ctx)
		env[k] = v;

	sol::protected_function_result pfr = lua.safe_script("return " + expr, env);
	if (!pfr.valid()) {
		sol::error err = pfr;
		LogError("SpellExpr eval error for '{}': {}", expr, err.what());
		return result;
	}

	if (pfr.get_type() == sol::type::table) {
		sol::table t = pfr;
		result.minValue = t.get_or("min", 0);
		result.maxValue = t.get_or("max", 0);
		result.value = result.minValue;
		result.isRange = true;
	} else if (pfr.get_type() == sol::type::number) {
		result.value = pfr.get<int>();
	}

	return result;
}

tl::expected<void, std::string> LoadSpellDescData()
{
	constexpr std::string_view filename = "txtdata\\spells\\spelldesc.tsv";
	SpellDescLines.clear();

	auto loadResult = DataFile::load(filename);
	if (!loadResult.has_value()) {
		return tl::make_unexpected(fmt::format("Failed to load {}", filename));
	}
	DataFile dataFile = std::move(*loadResult);

	dataFile.skipHeaderOrDie(filename);

	for (DataFileRecord record : dataFile) {
		// Collect all fields from the record to handle variable-width rows
		// (some rows have 4 fields, some have 6; trailing optional fields may be absent)
		std::vector<std::string> fields;
		for (DataFileField field : record) {
			fields.push_back(std::string(field.value()));
		}

		if (fields.size() < 4) {
			return tl::make_unexpected(fmt::format("{}: row has fewer than 4 columns", filename));
		}

		SpellDescLine line;
		line.spellId = ParseSpellIdForDesc(fields[0]);
		line.section = ParseDescSection(fields[1]);

		auto parseIntResult = std::from_chars(fields[2].data(), fields[2].data() + fields[2].size(), line.priority);
		if (parseIntResult.ec != std::errc()) {
			return tl::make_unexpected(fmt::format("{}: invalid priority '{}'", filename, fields[2]));
		}

		line.format = ParseDescFormat(fields[3]);

		if (fields.size() > 4)
			line.expression = fields[4];
		if (fields.size() > 5)
			line.textKey = fields[5];
		if (fields.size() > 6)
			line.formulaText = fields[6];

		SpellDescLines.push_back(std::move(line));
	}

	return {};
}

std::vector<const SpellDescLine *> GetSpellDescLines(SpellID spell, DescSection section)
{
	std::vector<const SpellDescLine *> result;
	for (const auto &line : SpellDescLines) {
		if (line.spellId == spell && line.section == section)
			result.push_back(&line);
	}
	std::sort(result.begin(), result.end(),
	    [](const SpellDescLine *a, const SpellDescLine *b) { return a->priority < b->priority; });
	return result;
}

std::string FormatDescLine(const SpellDescLine &line, const Player &player, SpellID spell, int level)
{
	switch (line.format) {
	case DescFormat::LevelDisplay:
		return fmt::format("Level {:d} / {:d}", level, MaxSpellLevel);

	case DescFormat::Special:
		return line.textKey;

	case DescFormat::Text:
		if (!line.textKey.empty())
			return line.textKey;
		return std::string(GetSpellData(spell).sDescription);

	case DescFormat::Mana: {
		int mana = EvaluateSpellExpr(line.expression.empty() ? "mana" : line.expression, player, spell, level).value;
		return fmt::format("{:s}: {:d}", line.textKey, mana);
	}

	case DescFormat::ManaDelta: {
		int curMana = EvaluateSpellExpr(line.expression.empty() ? "mana" : line.expression, player, spell, level).value;
		int nextMana = EvaluateSpellExpr(line.expression.empty() ? "mana" : line.expression, player, spell, level + 1).value;
		return fmt::format("{:s}: {:d} \xe2\x86\x92 {:d}", line.textKey, curMana, nextMana);
	}

	case DescFormat::DamageRange: {
		ExprResult r = EvaluateSpellExpr(line.expression, player, spell, level);
		if (r.isRange)
			return fmt::format("{:s}: {:d} - {:d}", line.textKey, r.minValue, r.maxValue);
		return fmt::format("{:s}: {:d}", line.textKey, r.value);
	}

	case DescFormat::HealRange: {
		ExprResult r = EvaluateSpellExpr(line.expression, player, spell, level);
		if (r.isRange)
			return fmt::format("{:s}: {:d} - {:d}", line.textKey, r.minValue, r.maxValue);
		return fmt::format("{:s}: {:d}", line.textKey, r.value);
	}

	case DescFormat::ValueSingle: {
		ExprResult r = EvaluateSpellExpr(line.expression, player, spell, level);
		return fmt::format("{:s}: {:d}", line.textKey, r.value);
	}

	case DescFormat::ValueDelta: {
		ExprResult cur = EvaluateSpellExpr(line.expression, player, spell, level);
		ExprResult next = EvaluateSpellExpr(line.expression, player, spell, level + 1);
		return fmt::format("{:s}: {:d} \xe2\x86\x92 {:d}", line.textKey, cur.value, next.value);
	}

	case DescFormat::HealDelta: {
		ExprResult cur = EvaluateSpellExpr(line.expression, player, spell, level);
		ExprResult next = EvaluateSpellExpr(line.expression, player, spell, level + 1);
		if (cur.isRange && next.isRange)
			return fmt::format("{:s}: {:d}-{:d} \xe2\x86\x92 {:d}-{:d}", line.textKey, cur.minValue, cur.maxValue, next.minValue, next.maxValue);
		return fmt::format("{:s}: {:d} \xe2\x86\x92 {:d}", line.textKey, cur.value, next.value);
	}
	}
	return "";
}

SpellTooltip BuildSpellTooltip(const Player &player, SpellID spell)
{
	const SpellData &sd = GetSpellData(spell);
	const int level = player.GetSpellLevel(spell);

	SpellTooltip tooltip;
	tooltip.title = pgettext("spell", sd.sNameText);

	// Desc section
	for (const auto *line : GetSpellDescLines(spell, DescSection::Desc)) {
		if (level == 0 && line->format != DescFormat::LevelDisplay && line->format != DescFormat::Text)
			continue; // Skip numeric lines at level 0
		if (level == 0 && line->format == DescFormat::LevelDisplay) {
			tooltip.lines.push_back(std::string(_("Spell Level 0 - Unusable")));
			continue;
		}
		tooltip.lines.push_back(FormatDescLine(*line, player, spell, level));
	}

	// Upgrade section (only if level > 0 and < max)
	if (level > 0 && level < MaxSpellLevel) {
		auto upgradeLines = GetSpellDescLines(spell, DescSection::Upgrade);
		if (!upgradeLines.empty()) {
			tooltip.lines.push_back(std::string(_("Next Level:")));
			for (const auto *line : upgradeLines) {
				tooltip.lines.push_back("  " + FormatDescLine(*line, player, spell, level));
			}
		}
	}

	// Warning section
	for (const auto *line : GetSpellDescLines(spell, DescSection::Warning)) {
		tooltip.lines.push_back(FormatDescLine(*line, player, spell, level));
	}

	return tooltip;
}

SpellTooltip BuildSpellListTooltip(const Player &player, SpellID spell)
{
	const SpellData &sd = GetSpellData(spell);
	const int level = player.GetSpellLevel(spell);

	SpellTooltip tooltip;
	tooltip.title = pgettext("spell", sd.sNameText);

	// Desc section only — no upgrade
	for (const auto *line : GetSpellDescLines(spell, DescSection::Desc)) {
		if (level == 0 && line->format != DescFormat::LevelDisplay && line->format != DescFormat::Text)
			continue; // Skip numeric lines at level 0
		if (level == 0 && line->format == DescFormat::LevelDisplay) {
			tooltip.lines.push_back(std::string(_("Spell Level 0 - Unusable")));
			continue;
		}
		tooltip.lines.push_back(FormatDescLine(*line, player, spell, level));
	}

	// Warning section
	for (const auto *line : GetSpellDescLines(spell, DescSection::Warning)) {
		tooltip.lines.push_back(FormatDescLine(*line, player, spell, level));
	}

	return tooltip;
}

} // namespace devilution
