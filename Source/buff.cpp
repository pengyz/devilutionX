#include "buff.h"

#include <algorithm>

namespace devilution {

bool Buffable::IsDebuff(BuffType type)
{
	switch (type) {
	case BuffType::Poison:
	case BuffType::Chill:
	case BuffType::Stun:
	case BuffType::Fear:
	case BuffType::Snare:
	case BuffType::LifeDrain:
	case BuffType::ManaBurn:
	case BuffType::Shock:
	case BuffType::SoulWeakened:
		return true;
	default:
		return false;
	}
}

void Buffable::Apply(BuffType type, int16_t value, int32_t duration, int32_t source)
{
	auto &targetList = IsDebuff(type) ? debuffs : buffs;
	size_t maxListSize = IsDebuff(type) ? MAX_DEBUFFS : MAX_BUFFS;

	for (auto &existing : targetList) {
		if (existing.type == type && existing.sourceEntity == source) {
			if (type == BuffType::Chill && existing.stacks < 3) {
				existing.stacks++;
				existing.duration = duration;
				return;
			}
			if (type == BuffType::Poison && existing.stacks < 5) {
				existing.stacks++;
				existing.duration = duration;
				return;
			}
			existing.duration = duration;
			existing.value = value;
			return;
		}
	}

	if (targetList.size() < maxListSize) {
		targetList.push_back({ type, value, duration, source, 1 });
	}
}

void Buffable::Remove(BuffType type)
{
	auto removeFrom = [type](auto &list) {
		list.erase(std::remove_if(list.begin(), list.end(),
		               [type](const BuffInstance &b) { return b.type == type; }),
		    list.end());
	};
	removeFrom(buffs);
	removeFrom(debuffs);
}

void Buffable::Process()
{
	auto process = [](auto &list) {
		for (auto it = list.begin(); it != list.end();) {
			if (--it->duration <= 0) {
				it = list.erase(it);
			} else {
				++it;
			}
		}
	};
	process(buffs);
	process(debuffs);
}

bool Buffable::Has(BuffType type) const
{
	for (auto &b : buffs)
		if (b.type == type) return true;
	for (auto &d : debuffs)
		if (d.type == type) return true;
	return false;
}

int16_t Buffable::GetValue(BuffType type) const
{
	int16_t total = 0;
	for (auto &b : buffs)
		if (b.type == type) total += b.value * b.stacks;
	for (auto &d : debuffs)
		if (d.type == type) total += d.value * d.stacks;
	return total;
}

} // namespace devilution
