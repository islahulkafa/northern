#include "widgets/accordion.hpp"
#include "widgets/container.hpp"
#include "widgets/label.hpp"
#include "widgets/widget_context.hpp"
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

    // Advance animation
    sec.updateAnimations(0.2f);
    EXPECT_FALSE(sec.hasActiveAnimations());
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
