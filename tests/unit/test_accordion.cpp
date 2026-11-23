#include "widgets/containers/accordion.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/label.hpp"
#include "widgets/base/widget_context.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(CollapsibleSectionTest, DefaultsAndToggle) {
    CollapsibleSection sec("Properties", true);
    EXPECT_EQ(sec.title(), "Properties");
    EXPECT_TRUE(sec.isExpanded());
    EXPECT_TRUE(sec.isFocusable());
    EXPECT_TRUE(sec.clipsChildren());

    int toggledCount = 0;
    bool lastState = false;
    sec.onToggled.connect([&](bool exp) {
        ++toggledCount;
        lastState = exp;
    });

    sec.setExpanded(false, false);
    EXPECT_FALSE(sec.isExpanded());
    EXPECT_EQ(toggledCount, 1);
    EXPECT_FALSE(lastState);
    EXPECT_FALSE(sec.body()->isVisible());

    sec.toggle();
    EXPECT_TRUE(sec.isExpanded());
    EXPECT_EQ(toggledCount, 2);
    EXPECT_TRUE(lastState);
    EXPECT_TRUE(sec.body()->isVisible());
}

TEST(CollapsibleSectionTest, ContentManagement) {
    CollapsibleSection sec("Inspector", true);
    auto label = std::make_unique<Label>("Content Text");
    Label* rawLabel = sec.setContent(std::move(label));

    EXPECT_EQ(sec.content(), rawLabel);
    EXPECT_EQ(rawLabel->parent(), sec.body());
}

TEST(CollapsibleSectionTest, MouseAndKeyboardToggle) {
    CollapsibleSection sec("Header", false);
    EXPECT_FALSE(sec.isExpanded());

    // Click header area (y <= 32)
    WidgetMouseEvent downEv{core::Point{50.0f, 16.0f}, platform::MouseButton::Left,
                            platform::KeyModifier::None};
    sec.onMouseDown(downEv);
    EXPECT_TRUE(downEv.isAccepted());

    WidgetMouseEvent upEv{core::Point{50.0f, 16.0f}, platform::MouseButton::Left,
                          platform::KeyModifier::None};
    sec.onMouseUp(upEv);
    EXPECT_TRUE(upEv.isAccepted());
    EXPECT_TRUE(sec.isExpanded());

    // Press Space key to collapse
    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    sec.onKeyDown(spaceEv);
    EXPECT_TRUE(spaceEv.isAccepted());
    EXPECT_FALSE(sec.isExpanded());
}

TEST(CollapsibleSectionTest, AnimatedExpansion) {
    CollapsibleSection sec("Animated", false);
    sec.setContent(std::make_unique<Label>("Item"));

    sec.setExpanded(true, true);
    EXPECT_TRUE(sec.isExpanded());
    EXPECT_TRUE(sec.hasActiveAnimations());
    EXPECT_TRUE(sec.body()->isVisible());

    // Advance animation past 0.25s duration
    sec.updateAnimations(0.3f);
    EXPECT_FALSE(sec.hasActiveAnimations());
}

TEST(CollapsibleSectionTest, SizeMetricsAndHeaderSizes) {
    CollapsibleSection secStd("Standard Section", true, nullptr, SectionHeaderSize::Standard);
    EXPECT_EQ(secStd.headerSize(), SectionHeaderSize::Standard);
    EXPECT_FLOAT_EQ(secStd.headerHeight(), 32.0f);

    secStd.setHeaderSize(SectionHeaderSize::Compact);
    EXPECT_EQ(secStd.headerSize(), SectionHeaderSize::Compact);
    EXPECT_FLOAT_EQ(secStd.headerHeight(), 28.0f);

    CollapsibleSection secCompact("Compact Section", false, nullptr, SectionHeaderSize::Compact);
    EXPECT_EQ(secCompact.headerSize(), SectionHeaderSize::Compact);
    EXPECT_FLOAT_EQ(secCompact.headerHeight(), 28.0f);
}

TEST(CollapsibleSectionTest, IndicatorPlacementAndRotation) {
    CollapsibleSection sec("Leading Section", false);
    EXPECT_EQ(sec.indicatorPlacement(), IndicatorPlacement::Leading);

    sec.setIndicatorPlacement(IndicatorPlacement::Trailing);
    EXPECT_EQ(sec.indicatorPlacement(), IndicatorPlacement::Trailing);

    renderer::PaintContext pctx;
    sec.layoutNode().computeLayout(300.0f, 100.0f);
    sec.paint(pctx);
    EXPECT_FALSE(pctx.commands().empty());
}

TEST(CollapsibleSectionTest, HeaderAccessoryManagement) {
    CollapsibleSection sec("Accessory Section", true);
    EXPECT_EQ(sec.headerAccessory(), nullptr);

    auto badge = std::make_unique<Label>("3");
    Label* rawBadge = sec.setHeaderAccessory(std::move(badge));
    EXPECT_EQ(sec.headerAccessory(), rawBadge);
    EXPECT_EQ(rawBadge->style().positionType, layout::PositionType::Absolute);

    // Replace accessory
    auto badge2 = std::make_unique<Label>("New");
    Label* rawBadge2 = sec.setHeaderAccessory(std::move(badge2));
    EXPECT_EQ(sec.headerAccessory(), rawBadge2);
}

TEST(CollapsibleSectionTest, ContentPadding) {
    CollapsibleSection sec("Padded Section", true);
    EXPECT_FLOAT_EQ(sec.contentPadding().left.value, 8.0f);
    EXPECT_FLOAT_EQ(sec.contentPadding().top.value, 4.0f);

    sec.setContentPadding(layout::RectLength::all(layout::Length::points(12.0f)));
    EXPECT_FLOAT_EQ(sec.contentPadding().left.value, 12.0f);
    EXPECT_FLOAT_EQ(sec.body()->style().padding.left.value, 12.0f);
}

TEST(AccordionTest, AddAndRemoveSections) {
    Accordion acc;
    EXPECT_EQ(acc.sectionCount(), 0u);
    EXPECT_FALSE(acc.isSingleExpanded());

    auto s1 = std::make_unique<CollapsibleSection>("Section 1", true);
    auto s2 = std::make_unique<CollapsibleSection>("Section 2", false);

    CollapsibleSection* raw1 = acc.addSection(std::move(s1));
    CollapsibleSection* raw2 = acc.addSection(std::move(s2));

    EXPECT_EQ(acc.sectionCount(), 2u);
    EXPECT_EQ(acc.sectionAt(0), raw1);
    EXPECT_EQ(acc.sectionAt(1), raw2);

    acc.removeSection(0);
    EXPECT_EQ(acc.sectionCount(), 1u);
    EXPECT_EQ(acc.sectionAt(0), raw2);

    acc.clearSections();
    EXPECT_EQ(acc.sectionCount(), 0u);
}

TEST(AccordionTest, SingleExpandedModeEnforcesMutualExclusion) {
    Accordion acc(true); // singleExpanded = true
    EXPECT_TRUE(acc.isSingleExpanded());

    auto s1 = std::make_unique<CollapsibleSection>("S1", true);
    auto s2 = std::make_unique<CollapsibleSection>("S2", false);

    CollapsibleSection* raw1 = acc.addSection(std::move(s1));
    CollapsibleSection* raw2 = acc.addSection(std::move(s2));

    EXPECT_TRUE(raw1->isExpanded());
    EXPECT_FALSE(raw2->isExpanded());

    // Expanding S2 should collapse S1
    raw2->setExpanded(true, false);
    EXPECT_TRUE(raw2->isExpanded());
    EXPECT_FALSE(raw1->isExpanded());
}

TEST(AccordionTest, DynamicIndexResolutionOnToggle) {
    Accordion acc;
    auto s1 = std::make_unique<CollapsibleSection>("S1", false);
    auto s2 = std::make_unique<CollapsibleSection>("S2", false);
    auto s3 = std::make_unique<CollapsibleSection>("S3", false);

    acc.addSection(std::move(s1));
    CollapsibleSection* raw2 = acc.addSection(std::move(s2));
    CollapsibleSection* raw3 = acc.addSection(std::move(s3));

    size_t lastToggledIdx = 999;
    bool lastToggledState = false;
    acc.onSectionToggled.connect([&](size_t idx, bool state) {
        lastToggledIdx = idx;
        lastToggledState = state;
    });

    // Remove first section (s1), shifting s2 to index 0 and s3 to index 1
    acc.removeSection(0);
    EXPECT_EQ(acc.sectionCount(), 2u);
    EXPECT_EQ(acc.sectionAt(0), raw2);
    EXPECT_EQ(acc.sectionAt(1), raw3);

    // Toggling s3 should emit dynamic index 1 (not stale index 2)
    raw3->setExpanded(true, false);
    EXPECT_EQ(lastToggledIdx, 1u);
    EXPECT_TRUE(lastToggledState);

    // Toggling s2 should emit dynamic index 0 (not stale index 1)
    raw2->setExpanded(true, false);
    EXPECT_EQ(lastToggledIdx, 0u);
    EXPECT_TRUE(lastToggledState);
}

TEST(AccordionTest, KeyboardArrowNavigation) {
    WidgetContext ctx;
    auto acc = std::make_unique<Accordion>();
    auto s1 = std::make_unique<CollapsibleSection>("S1", true);
    auto s2 = std::make_unique<CollapsibleSection>("S2", false);
    auto s3 = std::make_unique<CollapsibleSection>("S3", false);

    CollapsibleSection* raw1 = acc->addSection(std::move(s1));
    CollapsibleSection* raw2 = acc->addSection(std::move(s2));
    CollapsibleSection* raw3 = acc->addSection(std::move(s3));

    ctx.setRootWidget(std::move(acc));
    raw1->setFocus(FocusReason::Keyboard);
    EXPECT_TRUE(raw1->hasFocus());

    // Down arrow moves focus to S2
    WidgetKeyEvent downEv{platform::Key::Down, true, platform::KeyModifier::None};
    raw1->onKeyDown(downEv);
    EXPECT_TRUE(raw2->hasFocus());

    // Down arrow moves focus to S3
    raw2->onKeyDown(downEv);
    EXPECT_TRUE(raw3->hasFocus());

    // Up arrow moves focus back to S2
    WidgetKeyEvent upEv{platform::Key::Up, true, platform::KeyModifier::None};
    raw3->onKeyDown(upEv);
    EXPECT_TRUE(raw2->hasFocus());

    // Home moves focus to S1
    WidgetKeyEvent homeEv{platform::Key::Home, true, platform::KeyModifier::None};
    raw2->onKeyDown(homeEv);
    EXPECT_TRUE(raw1->hasFocus());

    // End moves focus to S3
    WidgetKeyEvent endEv{platform::Key::End, true, platform::KeyModifier::None};
    raw1->onKeyDown(endEv);
    EXPECT_TRUE(raw3->hasFocus());
}

TEST(AccordionTest, HeaderSizePropagation) {
    Accordion acc(false, SectionHeaderSize::Standard);
    auto s1 = std::make_unique<CollapsibleSection>("S1", true);
    CollapsibleSection* raw1 = acc.addSection(std::move(s1));
    EXPECT_EQ(raw1->headerSize(), SectionHeaderSize::Standard);

    acc.setHeaderSize(SectionHeaderSize::Compact);
    EXPECT_EQ(raw1->headerSize(), SectionHeaderSize::Compact);

    auto s2 = std::make_unique<CollapsibleSection>("S2", false);
    CollapsibleSection* raw2 = acc.addSection(std::move(s2));
    EXPECT_EQ(raw2->headerSize(), SectionHeaderSize::Compact);
}
