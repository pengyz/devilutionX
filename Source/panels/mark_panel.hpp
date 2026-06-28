#pragma once
#include "engine/rectangle.hpp"
#include "engine/surface.hpp"
namespace devilution {
extern bool IsMarkPanelOpen;
void DrawMarkPanel(const Surface &out);
Rectangle GetMarkPanelCloseRect();
void CheckMarkPanelButton();
}
