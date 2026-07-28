#include "lua/modules/spells.hpp"

#include <sol/sol.hpp>

#include "lua/metadoc.hpp"

namespace devilution {

sol::table LuaSpellsModule(sol::state_view &lua)
{
	sol::table table = lua.create_table();
	// The module is a placeholder — actual expression evaluation
	// happens via sol::state::safe_script with a custom environment.
	// This module exposes helper functions if needed by mods.
	return table;
}

} // namespace devilution
