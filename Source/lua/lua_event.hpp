#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace devilution {

struct Player;
struct Monster;
struct Quest;

namespace lua {

void MonsterDataLoaded();
void UniqueMonsterDataLoaded();
void ItemDataLoaded();
void UniqueItemDataLoaded();

void StoreOpened(std::string_view name);

void OnMonsterTakeDamage(const Monster *monster, int damage, int damageType);

void OnPlayerGainExperience(const Player *player, uint32_t exp);
void OnPlayerTakeDamage(const Player *player, int damage, int damageType);

void LoadModsComplete();
void GameDrawComplete();
void GameStart();

std::string OnQuestCheck(std::string_view scriptName, const Quest *quest);

} // namespace lua

} // namespace devilution
