#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "engine/point.hpp"
#include "engine/rectangle.hpp"

namespace devilution {

enum class FloatingPosition : uint8_t {
    Cursor,
    WorldEntity,
    PanelElement,
};

class FloatingInfoEngine {
public:
    void SetInfo(std::string_view text, FloatingPosition pos, Point anchor);
    void Clear();

    [[nodiscard]] std::string_view GetText() const { return text_; }
    [[nodiscard]] FloatingPosition GetPosition() const { return position_; }
    [[nodiscard]] bool IsEmpty() const { return text_.empty(); }
    [[nodiscard]] Point GetAnchor() const { return anchor_; }

    [[nodiscard]] Rectangle GetRect() const;
    void SetScreenBounds(int width, int height);

private:
    std::string text_;
    FloatingPosition position_ = FloatingPosition::Cursor;
    Point anchor_ { 0, 0 };
    int screenWidth_ = 640;
    int screenHeight_ = 480;

    [[nodiscard]] Size CalcTextSize() const;
};

} // namespace devilution
