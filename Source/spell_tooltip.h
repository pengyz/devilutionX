#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <expected>

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
	GolemHP,        // Golem hit points -> int
	GolemArmor,     // Golem armor class -> int
	GolemToHit,     // Golem attack accuracy -> int
	ChainRadius,    // Chain Lightning search radius (tiles) -> int
	ChainTargets,   // Chain Lightning max targets -> string (unlimited)
	FireWallLength, // Fire Wall length (tiles) -> int
	FireWallDur,    // Fire Wall duration per tile (ticks/16) -> int
	NovaRadius,     // Nova radius (tiles) -> int
	FlameWaveWidth, // Flame Wave width (tiles) -> int
	TeleportRange,  // Teleport range (tiles) -> int
	PhasingMinDist, // Phasing minimum distance (tiles) -> int
	TownPortalDur,  // Town Portal duration (ticks/16) -> int
	ResurrectHP,    // Resurrect HP restored -> int
	ReflectCount,   // Reflect hit count -> int
	ReflectPct,     // Reflect damage % -> string
	RingOfFireRadius, // Ring of Fire radius (tiles) -> int
	RageDuration,   // Rage duration (ticks/16) -> int
	RageHPCost,     // Rage HP cost -> int
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

extern DVL_API_FOR_TEST std::vector<SpellDescLine> SpellDescLines;

std::expected<void, std::string> LoadSpellDescData();
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
