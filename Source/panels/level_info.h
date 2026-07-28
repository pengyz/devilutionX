#pragma once

#include <string>

#include "engine/point.hpp"

namespace devilution {

struct Surface;

class LevelInfoBar {
public:
    void Draw(const Surface &out, Point basePosition);

private:
    [[nodiscard]] std::string GetLevelName() const;
    [[nodiscard]] std::string GetDifficultyText() const;
};

} // namespace devilution
