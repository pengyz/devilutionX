#include "panels/level_info.h"

#include <fmt/format.h>

#include "diablo.h"
#include "engine/render/text_render.hpp"
#include "engine/surface.hpp"
#include "game_mode.hpp"
#include "levels/gendung.h"
#include "options.h"
#include "utils/language.h"

namespace devilution {

namespace {
const char *TypeNames[] = {
    "",           // DTYPE_TOWN
    N_("Catacombs"),
    N_("Catacombs"),
    N_("Caves"),
    N_("Hell"),
    N_("Hive"),
    N_("Crypt"),
};
const char *DifficultyNames[] = { N_("Normal"), N_("Nightmare"), N_("Hell") };
} // namespace

std::string LevelInfoBar::GetLevelName() const
{
    if (currlevel == 0)
        return std::string(_("Tristram"));
    return fmt::format(fmt::runtime(_("{:s} Level {:d}")),
        _(TypeNames[static_cast<size_t>(leveltype)]), currlevel);
}

std::string LevelInfoBar::GetDifficultyText() const
{
    if (gbIsMultiplayer || currlevel == 0)
        return {};
    return std::string(_(DifficultyNames[static_cast<size_t>(sgGameInitInfo.nDifficulty)]));
}

void LevelInfoBar::Draw(const Surface &out, Point basePosition)
{
	if (!gbRunGame)
		return;

	constexpr int xOffset = 177;
    constexpr int yOffset = 50;
    constexpr int lineHeight = 14;
    constexpr int spacing = 2;

    std::string levelName = GetLevelName();
    DrawString(out, levelName,
        Rectangle { { basePosition.x + xOffset, basePosition.y + yOffset }, { 88, lineHeight } },
        { .flags = UiFlags::ColorButtonface, .spacing = spacing, .lineHeight = lineHeight });

    std::string difficulty = GetDifficultyText();
    if (!difficulty.empty()) {
        DrawString(out, difficulty,
            Rectangle { { basePosition.x + xOffset, basePosition.y + yOffset + lineHeight + 2 }, { 88, lineHeight } },
            { .flags = UiFlags::ColorWhitegold, .spacing = spacing, .lineHeight = lineHeight });
    }
}

} // namespace devilution
