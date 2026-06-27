#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "utils/enum_traits.h"

namespace devilution {

enum class BuffType : uint8_t {
	// Offensive
	DamageBoost,
	AttackSpeed,
	CritChance,

	// Defensive
	ArmorBoost,
	DamageReduction,
	BlockChance,
	Reflect,

	// Elemental / DOT
	Ignite,
	Poison,
	Chill,
	Shock,

	// Crowd Control
	Stun,
	Fear,
	Taunt,
	Snare,

	// Resource
	ManaRegen,
	LifeDrain,
	ManaBurn,

	// Special
	Invulnerable,
	Invisible,
	SoulWeakened,

	COUNT
};

struct BuffInstance {
	BuffType type;
	int16_t value;
	int32_t duration;
	int32_t sourceEntity;
	uint8_t stacks;
};

class Buffable {
public:
	void Process();
	void Apply(BuffType type, int16_t value, int32_t duration, int32_t source);
	void Remove(BuffType type);
	[[nodiscard]] bool Has(BuffType type) const;
	[[nodiscard]] int16_t GetValue(BuffType type) const;

	static constexpr size_t MAX_BUFFS = 16;
	static constexpr size_t MAX_DEBUFFS = 8;

	std::vector<BuffInstance> buffs;
	std::vector<BuffInstance> debuffs;

private:
	static bool IsDebuff(BuffType type);
};

} // namespace devilution
