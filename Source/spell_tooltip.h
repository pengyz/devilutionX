#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <expected.hpp>

#include "player.h"
#include "tables/spelldat.h"

namespace devilution {

struct ExprResult {
	int value;
	int minValue;
	int maxValue;
	bool isRange;
};

ExprResult EvaluateSpellExpr(const std::string &expr, const Player &player, SpellID spell, int level);

enum class DescFormat : uint8_t {
	DamageRange,
	HealRange,
	ValueSingle,
	ValueDelta,
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
	std::string expression;
	std::string textKey;
	std::string formulaText;
};

extern std::vector<SpellDescLine> SpellDescLines;

tl::expected<void, std::string> LoadSpellDescData();
std::vector<const SpellDescLine *> GetSpellDescLines(SpellID spell, DescSection section);
std::string FormatDescLine(const SpellDescLine &line, const Player &player, SpellID spell, int level);

struct SpellTooltip {
	std::string title;
	std::vector<std::string> lines;
};

SpellTooltip BuildSpellTooltip(const Player &player, SpellID spell);
SpellTooltip BuildSpellListTooltip(const Player &player, SpellID spell);

} // namespace devilution
