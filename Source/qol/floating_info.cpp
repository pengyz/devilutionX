#include "qol/floating_info.h"

#include <algorithm>
#include <cstdint>

namespace devilution {

void FloatingInfoEngine::SetInfo(std::string_view text, FloatingPosition pos, Point anchor)
{
    text_ = std::string(text);
    position_ = pos;
    anchor_ = anchor;
}

void FloatingInfoEngine::Clear()
{
    text_.clear();
    position_ = FloatingPosition::Cursor;
    anchor_ = { 0, 0 };
}

Size FloatingInfoEngine::CalcTextSize() const
{
    if (text_.empty())
        return { 0, 0 };

    constexpr int charWidth = 10;
    constexpr int lineHeight = 14;

    int maxLineWidth = 0;
    int lineWidth = 0;
    int lineCount = 1;
    for (char c : text_) {
        if (c == '\n') {
            maxLineWidth = std::max(maxLineWidth, lineWidth);
            lineWidth = 0;
            lineCount++;
        } else {
            lineWidth += charWidth;
        }
    }
    maxLineWidth = std::max(maxLineWidth, lineWidth);

    return { maxLineWidth + 10, lineCount * lineHeight + 8 };
}

Rectangle FloatingInfoEngine::GetRect() const
{
    if (text_.empty())
        return { { 0, 0 }, { 0, 0 } };

    Size textSize = CalcTextSize();
    int boxW = textSize.width;
    int boxH = textSize.height;

    int x = anchor_.x;
    int y = anchor_.y;

    switch (position_) {
    case FloatingPosition::Cursor:
        x = anchor_.x + 16;
        y = anchor_.y + 20;
        break;
    case FloatingPosition::WorldEntity:
        x = anchor_.x - boxW / 2;
        y = anchor_.y - boxH - 4;
        break;
    case FloatingPosition::PanelElement:
        x = anchor_.x - boxW / 2;
        y = anchor_.y - boxH - 4;
        break;
    }

    // Clamp to screen bounds
    x = std::clamp(x, 0, screenWidth_ - boxW);
    if (y < 0) {
        y = anchor_.y + 32; // flip below anchor
    }
    y = std::clamp(y, 0, screenHeight_ - boxH);

    return { { x, y }, { boxW, boxH } };
}

void FloatingInfoEngine::SetScreenBounds(int width, int height)
{
    screenWidth_ = width;
    screenHeight_ = height;
}

} // namespace devilution
