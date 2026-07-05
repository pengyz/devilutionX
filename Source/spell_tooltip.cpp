#include "spell_tooltip.h"

#include <cmath>
#include <string>

#include <fmt/format.h>
#include <sol/sol.hpp>

#include "missiles.h"
#include "spells.h"
#include "utils/log.hpp"

namespace devilution {

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
	ctx["mana"] = GetManaAmount(player, spell) >> 6;

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

} // namespace devilution
