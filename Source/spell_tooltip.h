#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <expected.hpp>

#include "engine/render/text_render.hpp"
#include "player.h"
#include "tables/spelldat.h"

namespace devilution {

enum class DescSource : uint8_t {
	None,           // No data source (text/special/level_display)
	Damage,         // GetDamageAmt(spell, level) -> {min, max}
	Mana,           // GetManaAmount(player, spell, level) -> int
	Absorb,         // ManaShield absorption % -> int
	HPDamage,       // ManaShield HP damage % -> int
	Duration,       // Spell duration in ticks/16 -> int
	Bolts,          // ChargedBolt projectile count -> int
	Speed,          // Projectile speed (px/tick) -> int
	GuardianLife,    // Guardian lifetime (ticks/16) -> int
};

enum class DescFormat : uint8_t {
	DamageRange,
	HealRange,
	ValueSingle,
	ValueDelta,
	DamageDelta,
	Mana,
	ManaDelta,
	Text,
	Special,
	LevelDisplay,
	HealDelta,
};

enum class DescSection : uint8_t {
	Desc,
	Upgrade,
	Warning,
};

struct SpellDescLine {
	SpellID spellId;
	DescSection section;
	uint8_t priority;
	DescFormat format;
	DescSource source;
	std::string textKey;
	std::string formulaText;
};

extern std::vector<SpellDescLine> SpellDescLines;

tl::expected<void, std::string> LoadSpellDescData();
std::vector<const SpellDescLine *> GetSpellDescLines(SpellID spell, DescSection section);
std::string FormatDescLine(const SpellDescLine &line, const Player &player, SpellID spell, int level);

struct TooltipLine {
	std::string text;
	UiFlags color;
};

struct SpellTooltip {
	std::string title;
	UiFlags titleColor;
	std::vector<std::pair<std::string, UiFlags>> lines;
};

SpellTooltip BuildSpellTooltip(const Player &player, SpellID spell);
SpellTooltip BuildSpellListTooltip(const Player &player, SpellID spell, SpellType type);

} // namespace devilution
