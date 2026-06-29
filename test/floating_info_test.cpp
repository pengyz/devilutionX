#include <gtest/gtest.h>

#include "qol/floating_info.h"

using namespace devilution;

TEST(FloatingInfoEngine, Init_CreatesDefaultState)
{
    FloatingInfoEngine engine;
    EXPECT_TRUE(engine.IsEmpty());
    EXPECT_EQ(engine.GetPosition(), FloatingPosition::Cursor);
}

TEST(FloatingInfoEngine, SetInfo_StoresCorrectly)
{
    FloatingInfoEngine engine;
    engine.SetInfo("Test Item", FloatingPosition::WorldEntity, { 100, 200 });
    EXPECT_EQ(engine.GetText(), "Test Item");
    EXPECT_FALSE(engine.IsEmpty());
    EXPECT_EQ(engine.GetPosition(), FloatingPosition::WorldEntity);
    EXPECT_EQ(engine.GetAnchor(), Point(100, 200));
}

TEST(FloatingInfoEngine, GetRect_EmptyString_ReturnsZeroSize)
{
    FloatingInfoEngine engine;
    auto rect = engine.GetRect();
    EXPECT_EQ(rect.size.width, 0);
    EXPECT_EQ(rect.size.height, 0);
}

TEST(FloatingInfoEngine, GetRect_CursorMode_OffsetFromAnchor)
{
    FloatingInfoEngine engine;
    engine.SetInfo("Test", FloatingPosition::Cursor, { 100, 100 });
    auto rect = engine.GetRect();
    EXPECT_GT(rect.position.x, 100); // should be to the right of anchor
    EXPECT_GT(rect.position.y, 100); // should be below anchor
    EXPECT_GT(rect.size.width, 0);
    EXPECT_GT(rect.size.height, 0);
}

TEST(FloatingInfoEngine, GetRect_WorldEntity_CenteredAbove)
{
    FloatingInfoEngine engine;
    engine.SetInfo("Monster Name", FloatingPosition::WorldEntity, { 320, 200 });
    auto rect = engine.GetRect();
    // Centered horizontally: rect center should be near anchor x
    int centerX = rect.position.x + rect.size.width / 2;
    EXPECT_NEAR(centerX, 320, rect.size.width);
    // Above anchor: rect bottom should be near anchor y
    int bottomY = rect.position.y + rect.size.height;
    EXPECT_LT(bottomY, 205); // above the anchor point
}

TEST(FloatingInfoEngine, GetRect_PanelElement_CenteredAbove)
{
    FloatingInfoEngine engine;
    engine.SetInfo("Button", FloatingPosition::PanelElement, { 200, 150 });
    auto rect = engine.GetRect();
    int centerX = rect.position.x + rect.size.width / 2;
    EXPECT_NEAR(centerX, 200, rect.size.width);
    int bottomY = rect.position.y + rect.size.height;
    EXPECT_LT(bottomY, 155);
}

TEST(FloatingInfoEngine, GetRect_ClampRightEdge)
{
    FloatingInfoEngine engine;
    // Anchor near right edge of 640-width screen
    engine.SetInfo("A long tooltip text that extends", FloatingPosition::Cursor, { 620, 200 });
    auto rect = engine.GetRect();
    EXPECT_LE(rect.position.x + rect.size.width, 640);
}

TEST(FloatingInfoEngine, GetRect_ClampBottomEdge)
{
    FloatingInfoEngine engine;
    // Anchor near bottom of 480-height screen
    engine.SetInfo("Tooltip", FloatingPosition::Cursor, { 100, 470 });
    auto rect = engine.GetRect();
    EXPECT_LE(rect.position.y + rect.size.height, 480);
}

TEST(FloatingInfoEngine, GetRect_TopEdge_FlipsBelow)
{
    FloatingInfoEngine engine;
    // Anchor near top - box would go off-screen, should flip below
    engine.SetInfo("Tooltip", FloatingPosition::WorldEntity, { 320, 2 });
    auto rect = engine.GetRect();
    EXPECT_GT(rect.position.y, 0);
}

TEST(FloatingInfoEngine, GetRect_Multiline_HeightProportional)
{
    FloatingInfoEngine engine;
    engine.SetInfo("Line 1\nLine 2\nLine 3", FloatingPosition::Cursor, { 100, 100 });
    auto rect = engine.GetRect();
    EXPECT_GT(rect.size.height, 30); // 3 lines should be tall
}

TEST(FloatingInfoEngine, SetScreenBounds_ChangesClampRegion)
{
    FloatingInfoEngine engine;
    engine.SetScreenBounds(320, 240); // Small screen
    engine.SetInfo("Tooltip", FloatingPosition::Cursor, { 300, 230 });
    auto rect = engine.GetRect();
    EXPECT_LE(rect.position.x + rect.size.width, 320);
    EXPECT_LE(rect.position.y + rect.size.height, 240);
}

TEST(FloatingInfoEngine, Clear_ResetsAllState)
{
    FloatingInfoEngine engine;
    engine.SetInfo("Some text", FloatingPosition::PanelElement, { 50, 50 });
    engine.Clear();
    EXPECT_TRUE(engine.IsEmpty());
    EXPECT_EQ(engine.GetPosition(), FloatingPosition::Cursor);
}
