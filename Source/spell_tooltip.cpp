#include "spell_tooltip.h"

#include <algorithm>
#include <charconv>
#include <string>

#ifdef USE_SDL3
#include <SDL3/SDL_keycode.h>
#else
#include <SDL.h>
#endif

#include <fmt/format.h>

#include "appfat.h"
#include "data/file.hpp"
#include "data/iterators.hpp"
#include "missiles.h"
#include "player.h"
#include "spells.h"
#include "utils/language.h"

namespace devilution {

std::vector<SpellDescLine> SpellDescLines;

namespace {

// ---- C++ helper functions for computed spell stats ----

DamageRange GetSpellDamage(SpellID spell, int level)
{
	return GetDamageAmt(spell, level);
}

int GetSpellMana(const Player &player, SpellID spell, int level)
{
	return GetManaAmount(player, spell, level) >> 6;
}

int GetManaShieldAbsorb(int level)
{
	// player.cpp:1720 — divisor = 24 - min(level, 7) * 3
	const int divisor = 24 - (std::min(level, 7) * 3);
	// Mana absorbs (divisor-1)/divisor of damage
	return (divisor - 1) * 100 / divisor;
}

int GetManaShieldHPDamage(int level)
{
	const int divisor = 24 - (std::min(level, 7) * 3);
	return 100 / divisor;
}

int GetStoneCurseDuration(int level)
{
	// missiles.cpp:2409 — duration = min(level + 6, 15) * 16 ticks
	return std::min(level + 6, 15);
}

int GetChargedBoltCount(int level, const SpellData &sd)
{
	// par1 + floor(level / 3)
	return sd.sParam[0] + level / 3;
}

int GetFireboltSpeed(int level)
{
	// missiles.cpp: speed = 16 + min(level * 2, 47)
	return 16 + std::min(level * 2, 47);
}

int GetFireballSpeed(int level)
{
	// missiles.cpp: speed = 16 + min(level * 2, 34)
	return 16 + std::min(level * 2, 34);
}

int GetGuardianLifetime(int level, int charLevel)
{
	// missiles.cpp:2223 — duration = min(level + charLevel/2, 30)
	return std::min(level + charLevel / 2, 30);
}

// ---- Get a single int value from source ----

int GetSourceValue(DescSource source, const Player &player, SpellID spell, int level)
{
	switch (source) {
	case DescSource::Mana:
		return GetSpellMana(player, spell, level);
	case DescSource::Absorb:
		return GetManaShieldAbsorb(level);
	case DescSource::HPDamage:
		return GetManaShieldHPDamage(level);
	case DescSource::Duration:
		return GetStoneCurseDuration(level);
	case DescSource::Bolts:
		return GetChargedBoltCount(level, GetSpellData(spell));
	case DescSource::Speed:
		if (spell == SpellID::Fireball) return GetFireballSpeed(level);
		return GetFireboltSpeed(level);
	case DescSource::GuardianLife:
		return GetGuardianLifetime(level, player.getCharacterLevel());
	default:
		return 0;
	}
}

// ---- Parsers ----

DescSource ParseDescSource(std::string_view value)
{
	if (value.empty() || value == "none") return DescSource::None;
	if (value == "damage") return DescSource::Damage;
	if (value == "mana") return DescSource::Mana;
	if (value == "absorb") return DescSource::Absorb;
	if (value == "hp_damage") return DescSource::HPDamage;
	if (value == "duration") return DescSource::Duration;
	if (value == "bolts") return DescSource::Bolts;
	if (value == "speed") return DescSource::Speed;
	if (value == "guardian_life") return DescSource::GuardianLife;
	app_fatal(fmt::format("Unknown DescSource: {}", value));
	return DescSource::None;
}

DescFormat ParseDescFormat(std::string_view value)
{
	if (value == "damage_range") return DescFormat::DamageRange;
	if (value == "heal_range") return DescFormat::HealRange;
	if (value == "value_single") return DescFormat::ValueSingle;
	if (value == "value_delta") return DescFormat::ValueDelta;
	if (value == "damage_delta") return DescFormat::DamageDelta;
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
			line.source = ParseDescSource(fields[4]);
		else
			line.source = DescSource::None;

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
	// Translate textKey for display
	const char *label = line.textKey.empty() ? "" : pgettext("spell_tooltip", line.textKey.c_str());

	// Alt key: show formula text if available
	if (!line.formulaText.empty() && (SDL_GetModState() & KMOD_ALT) != 0) {
		return fmt::format("{:s}: {:s}", label, line.formulaText);
	}

	switch (line.format) {
	case DescFormat::LevelDisplay:
		return fmt::format(fmt::runtime(_("Level {:d} / {:d}")), level, MaxSpellLevel);

	case DescFormat::Special:
		return label;

	case DescFormat::Text:
		if (!line.textKey.empty())
			return label;
		return std::string(GetSpellData(spell).sDescription);

	case DescFormat::Mana: {
		int mana = GetSourceValue(line.source, player, spell, level);
		return fmt::format("{:s}: {:d}", label, mana);
	}

	case DescFormat::ManaDelta: {
		int curMana = GetSourceValue(line.source, player, spell, level);
		int nextMana = GetSourceValue(line.source, player, spell, level + 1);
		return fmt::format("{:s}: {:d} -> {:d}", label, curMana, nextMana);
	}

	case DescFormat::DamageRange: {
		DamageRange dr = GetSpellDamage(spell, level);
		return fmt::format("{:s}: {:d} - {:d}", label, dr.min, dr.max);
	}

	case DescFormat::HealRange: {
		DamageRange dr = GetSpellDamage(spell, level);
		return fmt::format("{:s}: {:d} - {:d}", label, dr.min, dr.max);
	}

	case DescFormat::ValueSingle: {
		int val = GetSourceValue(line.source, player, spell, level);
		return fmt::format("{:s}: {:d}", label, val);
	}

	case DescFormat::ValueDelta: {
		int cur = GetSourceValue(line.source, player, spell, level);
		int next = GetSourceValue(line.source, player, spell, level + 1);
		return fmt::format("{:s}: {:d} -> {:d}", label, cur, next);
	}

	case DescFormat::DamageDelta: {
		DamageRange cur = GetSpellDamage(spell, level);
		DamageRange next = GetSpellDamage(spell, level + 1);
		return fmt::format("{:s}: {:d}-{:d} -> {:d}-{:d}", label, cur.min, cur.max, next.min, next.max);
	}

	case DescFormat::HealDelta: {
		DamageRange cur = GetSpellDamage(spell, level);
		DamageRange next = GetSpellDamage(spell, level + 1);
		return fmt::format("{:s}: {:d}-{:d} -> {:d}-{:d}", label, cur.min, cur.max, next.min, next.max);
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
			continue;
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
			continue;
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
