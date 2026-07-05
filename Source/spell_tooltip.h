#pragma once

#include <cstdint>
#include <string>

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

} // namespace devilution
