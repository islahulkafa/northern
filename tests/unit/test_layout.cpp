#include "layout/layout_node.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::layout;

TEST(LayoutTest, LengthResolution) {
    Length p = Length::points(50.0f);
    EXPECT_TRUE(p.isPoints());
    EXPECT_FLOAT_EQ(p.resolve(200.0f), 50.0f);

    Length pct = Length::percent(25.0f);
    EXPECT_TRUE(pct.isPercent());
    EXPECT_FLOAT_EQ(pct.resolve(200.0f), 50.0f);

    Length a = Length::autoLength();
    EXPECT_TRUE(a.isAuto());
    EXPECT_FLOAT_EQ(a.resolve(200.0f, 15.0f), 15.0f);
}

TEST(LayoutTest, FlexDirectionRowAndColumn) {
    // Row layout
    {
        LayoutNode root;
        root.style().width = Length::points(300.0f);
        root.style().height = Length::points(100.0f);
        root.style().flexDirection = FlexDirection::Row;

        auto c1 = std::make_unique<LayoutNode>();
        c1->style().width = Length::points(100.0f);
        c1->style().height = Length::points(50.0f);
        root.addChild(std::move(c1));

        auto c2 = std::make_unique<LayoutNode>();
        c2->style().width = Length::points(100.0f);
        c2->style().height = Length::points(50.0f);
        root.addChild(std::move(c2));

        root.computeLayout(300.0f, 100.0f);

        EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().origin.x, 0.0f);
        EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.width, 100.0f);
        EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().origin.x, 100.0f);
        EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().size.width, 100.0f);
    }

    // Column layout
    {
        LayoutNode root;
        root.style().width = Length::points(100.0f);
        root.style().height = Length::points(300.0f);
        root.style().flexDirection = FlexDirection::Column;

        auto c1 = std::make_unique<LayoutNode>();
        c1->style().width = Length::points(80.0f);
        c1->style().height = Length::points(60.0f);
        root.addChild(std::move(c1));

        auto c2 = std::make_unique<LayoutNode>();
        c2->style().width = Length::points(80.0f);
        c2->style().height = Length::points(40.0f);
        root.addChild(std::move(c2));

        root.computeLayout(100.0f, 300.0f);

        EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().origin.y, 0.0f);
        EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.height, 60.0f);
        EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().origin.y, 60.0f);
        EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().size.height, 40.0f);
    }
}

TEST(LayoutTest, JustifyContentOptions) {
    auto setupContainer = [](JustifyContent jc) {
        LayoutNode root;
        root.style().width = Length::points(300.0f);
        root.style().height = Length::points(100.0f);
        root.style().flexDirection = FlexDirection::Row;
        root.style().justifyContent = jc;

        auto c1 = std::make_unique<LayoutNode>();
        c1->style().width = Length::points(60.0f);
        c1->style().height = Length::points(40.0f);
        root.addChild(std::move(c1));

        auto c2 = std::make_unique<LayoutNode>();
        c2->style().width = Length::points(40.0f);
        c2->style().height = Length::points(40.0f);
        root.addChild(std::move(c2));

        root.computeLayout(300.0f, 100.0f);
        return root;
    };

    // Center: total items = 100, remaining = 200, start = 100
    {
        auto root = setupContainer(JustifyContent::Center);
        EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().origin.x, 100.0f);
        EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().origin.x, 160.0f);
    }

    // FlexEnd: remaining = 200, start = 200
    {
        auto root = setupContainer(JustifyContent::FlexEnd);
        EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().origin.x, 200.0f);
        EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().origin.x, 260.0f);
    }

    // SpaceBetween: c1 at 0, c2 at 300 - 40 = 260
    {
        auto root = setupContainer(JustifyContent::SpaceBetween);
        EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().origin.x, 0.0f);
        EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().origin.x, 260.0f);
    }

    // SpaceEvenly: 3 spaces of 200 / 3 = 66.6667
    {
        auto root = setupContainer(JustifyContent::SpaceEvenly);
        float step = 200.0f / 3.0f;
        EXPECT_NEAR(root.childAt(0)->layoutRect().origin.x, step, 0.01f);
        EXPECT_NEAR(root.childAt(1)->layoutRect().origin.x, step + 60.0f + step, 0.01f);
    }
}

TEST(LayoutTest, AlignItemsAndAlignSelf) {
    LayoutNode root;
    root.style().width = Length::points(200.0f);
    root.style().height = Length::points(100.0f);
    root.style().flexDirection = FlexDirection::Row;
    root.style().alignItems = AlignItems::Center;

    auto c1 = std::make_unique<LayoutNode>();
    c1->style().width = Length::points(50.0f);
    c1->style().height = Length::points(40.0f);
    root.addChild(std::move(c1));

    // c2 overrides with AlignSelf::FlexEnd
    auto c2 = std::make_unique<LayoutNode>();
    c2->style().width = Length::points(50.0f);
    c2->style().height = Length::points(30.0f);
    c2->style().alignSelf = AlignSelf::FlexEnd;
    root.addChild(std::move(c2));

    // c3 overrides with AlignSelf::Stretch (auto height)
    auto c3 = std::make_unique<LayoutNode>();
    c3->style().width = Length::points(50.0f);
    c3->style().alignSelf = AlignSelf::Stretch;
    root.addChild(std::move(c3));

    root.computeLayout(200.0f, 100.0f);

    // c1 is centered: y = (100 - 40) / 2 = 30
    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().origin.y, 30.0f);

    // c2 is at FlexEnd: y = 100 - 30 = 70
    EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().origin.y, 70.0f);

    // c3 is Stretched: height = 100, y = 0
    EXPECT_FLOAT_EQ(root.childAt(2)->layoutRect().origin.y, 0.0f);
    EXPECT_FLOAT_EQ(root.childAt(2)->layoutRect().size.height, 100.0f);
}

TEST(LayoutTest, FlexGrowAndShrink) {
    // FlexGrow
    {
        LayoutNode root;
        root.style().width = Length::points(300.0f);
        root.style().height = Length::points(50.0f);
        root.style().flexDirection = FlexDirection::Row;

        auto c1 = std::make_unique<LayoutNode>();
        c1->style().width = Length::points(100.0f);
        c1->style().flexGrow = 1.0f;
        root.addChild(std::move(c1));

        auto c2 = std::make_unique<LayoutNode>();
        c2->style().width = Length::points(100.0f);
        c2->style().flexGrow = 1.0f;
        root.addChild(std::move(c2));

        root.computeLayout(300.0f, 50.0f);

        // 100px free space distributed equally -> each gets 150px
        EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.width, 150.0f);
        EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().size.width, 150.0f);
    }

    // FlexShrink
    {
        LayoutNode root;
        root.style().width = Length::points(200.0f);
        root.style().height = Length::points(50.0f);
        root.style().flexDirection = FlexDirection::Row;

        auto c1 = std::make_unique<LayoutNode>();
        c1->style().width = Length::points(150.0f);
        c1->style().flexShrink = 1.0f;
        root.addChild(std::move(c1));

        auto c2 = std::make_unique<LayoutNode>();
        c2->style().width = Length::points(150.0f);
        c2->style().flexShrink = 1.0f;
        root.addChild(std::move(c2));

        root.computeLayout(200.0f, 50.0f);

        // 100px deficit shrunk equally -> each becomes 100px
        EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.width, 100.0f);
        EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().size.width, 100.0f);
    }
}

TEST(LayoutTest, MinMaxConstraints) {
    LayoutNode root;
    root.style().width = Length::points(400.0f);
    root.style().height = Length::points(50.0f);
    root.style().flexDirection = FlexDirection::Row;

    auto c1 = std::make_unique<LayoutNode>();
    c1->style().width = Length::points(50.0f);
    c1->style().flexGrow = 1.0f;
    c1->style().maxWidth = Length::points(120.0f); // Clamped at 120
    root.addChild(std::move(c1));

    auto c2 = std::make_unique<LayoutNode>();
    c2->style().width = Length::points(50.0f);
    c2->style().flexGrow = 1.0f;
    root.addChild(std::move(c2));

    root.computeLayout(400.0f, 50.0f);

    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.width, 120.0f);
    // Remaining free space goes to c2: 400 - 120 = 280
    EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().size.width, 280.0f);
}

TEST(LayoutTest, MultiLineWrapAndGap) {
    LayoutNode root;
    root.style().width = Length::points(200.0f);
    root.style().flexWrap = FlexWrap::Wrap;
    root.style().columnGap = 10.0f;
    root.style().rowGap = 20.0f;

    for (int i = 0; i < 3; ++i) {
        auto c = std::make_unique<LayoutNode>();
        c->style().width = Length::points(95.0f);
        c->style().height = Length::points(40.0f);
        root.addChild(std::move(c));
    }

    root.computeLayout(200.0f, 0.0f);

    // Line 1: Item 0 at x=0, y=0. Item 1 at x=95+10=105, y=0. Total width 200.
    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().origin.x, 0.0f);
    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().origin.y, 0.0f);
    EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().origin.x, 105.0f);
    EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().origin.y, 0.0f);

    // Line 2: Item 2 wraps! x=0, y=40 (line 1 height) + 20 (row gap) = 60
    EXPECT_FLOAT_EQ(root.childAt(2)->layoutRect().origin.x, 0.0f);
    EXPECT_FLOAT_EQ(root.childAt(2)->layoutRect().origin.y, 60.0f);

    // Total height = 40 + 20 + 40 = 100
    EXPECT_FLOAT_EQ(root.layoutRect().size.height, 100.0f);
}

TEST(LayoutTest, MeasureFuncIntegration) {
    LayoutNode root;
    root.style().width = Length::points(300.0f);
    root.style().flexDirection = FlexDirection::Column;
    root.style().alignItems = AlignItems::FlexStart;

    auto textNode = std::make_unique<LayoutNode>();
    textNode->setMeasureFunc([](const LayoutNode&, float availW, float) {
        if (availW < 100.0f) {
            return core::Size{availW, 60.0f}; // 3 lines
        }
        return core::Size{180.0f, 24.0f}; // 1 line
    });

    root.addChild(std::move(textNode));
    root.computeLayout(300.0f, 0.0f);

    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.width, 180.0f);
    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.height, 24.0f);
}

TEST(LayoutTest, AbsolutePositioning) {
    LayoutNode root;
    root.style().width = Length::points(200.0f);
    root.style().height = Length::points(150.0f);
    root.style().padding = RectLength::all(Length::points(10.0f));

    auto inFlow = std::make_unique<LayoutNode>();
    inFlow->style().width = Length::points(50.0f);
    inFlow->style().height = Length::points(50.0f);
    root.addChild(std::move(inFlow));

    auto absChild = std::make_unique<LayoutNode>();
    absChild->style().positionType = PositionType::Absolute;
    absChild->style().width = Length::points(30.0f);
    absChild->style().height = Length::points(30.0f);
    absChild->style().position.right = Length::points(15.0f);
    absChild->style().position.bottom = Length::points(10.0f);
    root.addChild(std::move(absChild));

    root.computeLayout(200.0f, 150.0f);

    // In-flow child is at padding: (10, 10)
    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().origin.x, 10.0f);
    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().origin.y, 10.0f);

    // Absolute child is positioned from right and bottom:
    // x = 200 - 15 - 30 = 155
    // y = 150 - 10 - 30 = 110
    EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().origin.x, 155.0f);
    EXPECT_FLOAT_EQ(root.childAt(1)->layoutRect().origin.y, 110.0f);
}

TEST(LayoutTest, WindowingExampleHierarchy) {
    float winW = 1024.0f;
    float winH = 768.0f;
    float dpi = 1.0f;

    LayoutNode rootLayout;
    rootLayout.style().width = Length::points(winW);
    rootLayout.style().height = Length::points(winH);
    rootLayout.style().flexDirection = FlexDirection::Column;
    rootLayout.style().justifyContent = JustifyContent::Center;
    rootLayout.style().alignItems = AlignItems::Center;

    auto cardNode = std::make_unique<LayoutNode>();
    cardNode->style().width = Length::points(std::min(480.0f * dpi, winW * 0.88f));
    cardNode->style().flexDirection = FlexDirection::Column;
    cardNode->style().alignItems = AlignItems::Center;
    cardNode->style().padding = RectLength{
        Length::points(32.0f * dpi), // left
        Length::points(28.0f * dpi), // top
        Length::points(32.0f * dpi), // right
        Length::points(32.0f * dpi)  // bottom
    };
    cardNode->style().rowGap = 16.0f * dpi;

    auto badgeNode = std::make_unique<LayoutNode>();
    badgeNode->style().width = Length::points(160.0f * dpi);
    badgeNode->style().height = Length::points(28.0f * dpi);

    auto innerNode = std::make_unique<LayoutNode>();
    innerNode->style().width = Length::percent(100.0f);
    innerNode->style().height = Length::points(104.0f * dpi);

    auto btnNode = std::make_unique<LayoutNode>();
    btnNode->style().width = Length::points(180.0f * dpi);
    btnNode->style().height = Length::points(42.0f * dpi);
    btnNode->style().margin.top = Length::points(8.0f * dpi);

    LayoutNode* badgePtr = cardNode->addChild(std::move(badgeNode));
    LayoutNode* innerPtr = cardNode->addChild(std::move(innerNode));
    LayoutNode* btnPtr = cardNode->addChild(std::move(btnNode));
    LayoutNode* cardPtr = rootLayout.addChild(std::move(cardNode));

    rootLayout.computeLayout(winW, winH);

    core::Rect cardRect = cardPtr->layoutRect();
    core::Rect badgeRect{cardRect.origin + badgePtr->layoutRect().origin,
                         badgePtr->layoutRect().size};
    core::Rect innerRect{cardRect.origin + innerPtr->layoutRect().origin,
                         innerPtr->layoutRect().size};
    core::Rect btnRect{cardRect.origin + btnPtr->layoutRect().origin, btnPtr->layoutRect().size};

    std::cout << "[DEBUG] cardRect:  origin=(" << cardRect.origin.x << ", " << cardRect.origin.y
              << ") size=(" << cardRect.size.width << ", " << cardRect.size.height << ")\n";
    std::cout << "[DEBUG] badgeRect: origin=(" << badgeRect.origin.x << ", " << badgeRect.origin.y
              << ") size=(" << badgeRect.size.width << ", " << badgeRect.size.height << ")\n";
    std::cout << "[DEBUG] innerRect: origin=(" << innerRect.origin.x << ", " << innerRect.origin.y
              << ") size=(" << innerRect.size.width << ", " << innerRect.size.height << ")\n";
    std::cout << "[DEBUG] btnRect:   origin=(" << btnRect.origin.x << ", " << btnRect.origin.y
              << ") size=(" << btnRect.size.width << ", " << btnRect.size.height << ")\n";

    EXPECT_GT(cardRect.size.width, 0.0f);
    EXPECT_GT(cardRect.size.height, 0.0f);
    EXPECT_GT(innerRect.size.width, 0.0f);
    EXPECT_GT(innerRect.size.height, 0.0f);
}

TEST(LayoutTest, AspectRatioWidthToHeight) {
    LayoutNode root;
    root.style().width = Length::points(300.0f);
    root.style().height = Length::points(300.0f);
    root.style().flexDirection = FlexDirection::Row;
    root.style().alignItems = AlignItems::FlexStart;

    auto child = std::make_unique<LayoutNode>();
    child->style().width = Length::points(160.0f);
    child->style().height = Length::autoLength();
    child->style().aspectRatio = 16.0f / 9.0f;
    root.addChild(std::move(child));

    root.computeLayout(300.0f, 300.0f);

    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.width, 160.0f);
    EXPECT_NEAR(root.childAt(0)->layoutRect().size.height, 90.0f, 0.001f);
}

TEST(LayoutTest, AspectRatioHeightToWidth) {
    LayoutNode root;
    root.style().width = Length::points(400.0f);
    root.style().height = Length::points(200.0f);
    root.style().flexDirection = FlexDirection::Row;
    root.style().alignItems = AlignItems::FlexStart;

    auto child = std::make_unique<LayoutNode>();
    child->style().width = Length::autoLength();
    child->style().height = Length::points(100.0f);
    child->style().aspectRatio = 2.0f;
    root.addChild(std::move(child));

    root.computeLayout(400.0f, 200.0f);

    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.width, 200.0f);
    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.height, 100.0f);
}

TEST(LayoutTest, AspectRatioClampedByMinMax) {
    LayoutNode root;
    root.style().width = Length::points(400.0f);
    root.style().height = Length::points(400.0f);
    root.style().alignItems = AlignItems::FlexStart;

    auto child = std::make_unique<LayoutNode>();
    child->style().width = Length::points(200.0f);
    child->style().height = Length::autoLength();
    child->style().aspectRatio = 1.0f;
    child->style().maxHeight = Length::points(120.0f);
    root.addChild(std::move(child));

    root.computeLayout(400.0f, 400.0f);

    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.width, 200.0f);
    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.height, 120.0f);
}

TEST(LayoutTest, SubtreeLayoutCaching) {
    LayoutNode root;
    root.style().width = Length::points(300.0f);
    root.style().height = Length::points(200.0f);

    auto child = std::make_unique<LayoutNode>();
    child->style().width = Length::points(100.0f);
    child->style().height = Length::points(50.0f);
    LayoutNode* childPtr = root.addChild(std::move(child));

    EXPECT_TRUE(root.isDirty());
    root.computeLayout(300.0f, 200.0f);
    EXPECT_FALSE(root.isDirty());
    EXPECT_FLOAT_EQ(childPtr->layoutRect().size.width, 100.0f);

    // Call computeLayout again with identical bounds: cache hit short-circuits
    root.computeLayout(300.0f, 200.0f);
    EXPECT_FALSE(root.isDirty());

    // Modifying child style invalidates root dirty bit
    childPtr->style().width = Length::points(150.0f);
    childPtr->markDirty();
    EXPECT_TRUE(root.isDirty());

    root.computeLayout(300.0f, 200.0f);
    EXPECT_FALSE(root.isDirty());
    EXPECT_FLOAT_EQ(childPtr->layoutRect().size.width, 150.0f);
}

TEST(LayoutTest, MeasureFunctionCaching) {
    LayoutNode node;
    int measureCount = 0;
    node.setMeasureFunc([&measureCount](const LayoutNode&, float, float) {
        ++measureCount;
        return core::Size{80.0f, 24.0f};
    });

    EXPECT_EQ(measureCount, 0);

    core::Size s1 = node.measure(100.0f, 50.0f);
    EXPECT_EQ(measureCount, 1);
    EXPECT_FLOAT_EQ(s1.width, 80.0f);
    EXPECT_FLOAT_EQ(s1.height, 24.0f);

    // Identical arguments hit cache
    core::Size s2 = node.measure(100.0f, 50.0f);
    EXPECT_EQ(measureCount, 1);
    EXPECT_FLOAT_EQ(s2.width, 80.0f);

    // Different arguments invoke callback
    core::Size s3 = node.measure(200.0f, 50.0f);
    EXPECT_EQ(measureCount, 2);

    // Prior arguments hit MRU cache
    core::Size s4 = node.measure(100.0f, 50.0f);
    EXPECT_EQ(measureCount, 2);

    // markDirty invalidates cache
    node.markDirty();
    core::Size s5 = node.measure(100.0f, 50.0f);
    EXPECT_EQ(measureCount, 3);
}

TEST(LayoutTest, ScratchArenaStability) {
    LayoutNode root;
    root.style().width = Length::points(600.0f);
    root.style().height = Length::points(400.0f);
    root.style().flexWrap = FlexWrap::Wrap;

    for (int i = 0; i < 20; ++i) {
        auto child = std::make_unique<LayoutNode>();
        child->style().width = Length::points(50.0f);
        child->style().height = Length::points(30.0f);
        root.addChild(std::move(child));
    }

    for (int cycle = 0; cycle < 500; ++cycle) {
        root.markDirty();
        root.computeLayout(600.0f, 400.0f);
    }

    EXPECT_FLOAT_EQ(root.childAt(0)->layoutRect().size.width, 50.0f);
    EXPECT_FLOAT_EQ(root.childAt(19)->layoutRect().size.width, 50.0f);
}
