#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/accordion.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/label.hpp"
#include <gtest/gtest.h>
#include <variant>

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
    auto* row = sec.addRow("Item");

    sec.setExpanded(true, true);
    EXPECT_TRUE(sec.isExpanded());
    EXPECT_TRUE(sec.hasActiveAnimations());
    EXPECT_TRUE(sec.body()->isVisible());
    EXPECT_TRUE(sec.body()->clipsChildren());
    EXPECT_FLOAT_EQ(row->style().flexShrink, 0.0f);

    // Mid-animation: child row retains full 36px height without squishing
    sec.updateAnimations(0.12f);
    sec.layoutNode().computeLayout(320.0f, 300.0f);
    EXPECT_TRUE(sec.hasActiveAnimations());
    EXPECT_FLOAT_EQ(row->layoutRect().size.height, 36.0f);

    // Advance animation past 0.25s duration
    sec.updateAnimations(0.2f);
    EXPECT_FALSE(sec.hasActiveAnimations());
}

TEST(CollapsibleSectionTest, SizeMetricsAndHeaderSizes) {
    CollapsibleSection secStd("Standard Section", true, nullptr, SectionHeaderSize::Standard);
    EXPECT_EQ(secStd.headerSize(), SectionHeaderSize::Standard);
    EXPECT_FLOAT_EQ(secStd.headerHeight(), 38.0f);

    secStd.setHeaderSize(SectionHeaderSize::Compact);
    EXPECT_EQ(secStd.headerSize(), SectionHeaderSize::Compact);
    EXPECT_FLOAT_EQ(secStd.headerHeight(), 30.0f);

    CollapsibleSection secCompact("Compact Section", false, nullptr, SectionHeaderSize::Compact);
    EXPECT_EQ(secCompact.headerSize(), SectionHeaderSize::Compact);
    EXPECT_FLOAT_EQ(secCompact.headerHeight(), 30.0f);
}

TEST(CollapsibleSectionTest, IndicatorPlacementAndRotation) {
    CollapsibleSection sec("Trailing Section", false);
    EXPECT_EQ(sec.indicatorPlacement(), IndicatorPlacement::Trailing);

    sec.setIndicatorPlacement(IndicatorPlacement::Leading);
    EXPECT_EQ(sec.indicatorPlacement(), IndicatorPlacement::Leading);

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
    EXPECT_FLOAT_EQ(sec.contentPadding().left.value, 20.0f);
    EXPECT_FLOAT_EQ(sec.contentPadding().top.value, 2.0f);

    sec.setContentPadding(layout::RectLength::all(layout::Length::points(12.0f)));
    EXPECT_FLOAT_EQ(sec.contentPadding().left.value, 12.0f);
    EXPECT_FLOAT_EQ(sec.body()->style().padding.left.value, 12.0f);
}

TEST(CollapsibleSectionTest, DisclosureGroupRowsDividersAndDpi) {
    DisclosureGroup group("Display", true);
    group.setDpiScale(1.5f);
    group.setShowRowDividers(true);
    group.setCardStyled(true);

    EXPECT_FLOAT_EQ(group.dpiScale(), 1.5f);
    EXPECT_TRUE(group.showRowDividers());
    EXPECT_TRUE(group.isCardStyled());
    EXPECT_NE(group.effectiveFont(), nullptr);

    auto* row1 = group.addRow("Foo");
    auto* row2 = group.addRow("Bar");
    EXPECT_NE(row1, nullptr);
    EXPECT_NE(row2, nullptr);
    EXPECT_EQ(group.body()->childCount(), 2u);

    renderer::PaintContext pctx;
    group.layoutNode().computeLayout(360.0f, 300.0f);
    group.paint(pctx);
    EXPECT_FALSE(pctx.commands().empty());
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

TEST(AccordionTest, StylesAndDpiPropagation) {
    Accordion acc;
    EXPECT_EQ(acc.accordionStyle(), AccordionStyle::Grouped);

    auto* s1 = acc.addSection("Header", nullptr, true);
    auto* s2 = acc.addSection("Second", nullptr, false);
    acc.setDpiScale(1.25f);

    EXPECT_FLOAT_EQ(acc.dpiScale(), 1.25f);
    EXPECT_FLOAT_EQ(s1->dpiScale(), 1.25f);
    EXPECT_FLOAT_EQ(s2->dpiScale(), 1.25f);

    acc.setAccordionStyle(AccordionStyle::Separated);
    EXPECT_EQ(acc.accordionStyle(), AccordionStyle::Separated);

    acc.setAccordionStyle(AccordionStyle::Flush);
    EXPECT_EQ(acc.accordionStyle(), AccordionStyle::Flush);
}

TEST(CollapsibleSectionTest, DisabledIgnoresInteractionAndSuppressesHoverPill) {
    WidgetContext ctx;
    auto sec = std::make_unique<CollapsibleSection>("Disabled Section", false);
    auto* secPtr = sec.get();
    ctx.setRootWidget(std::move(sec));

    renderer::PaintContext initPaint;
    ctx.render(initPaint, {320.0f, 120.0f});

    platform::MouseEvent moveEv{};
    moveEv.type = platform::MouseEventType::Move;
    moveEv.physicalPosition = {24.0f, 16.0f};
    moveEv.position = moveEv.physicalPosition;
    ctx.handleMouseMove(moveEv);
    EXPECT_TRUE(secPtr->isHovered());

    platform::MouseEvent downEv{};
    downEv.type = platform::MouseEventType::ButtonDown;
    downEv.button = platform::MouseButton::Left;
    downEv.physicalPosition = {24.0f, 16.0f};
    downEv.position = downEv.physicalPosition;
    ctx.handleMouseDown(downEv);

    secPtr->setEnabled(false);
    EXPECT_FALSE(secPtr->isHovered());

    platform::MouseEvent upEv{};
    upEv.type = platform::MouseEventType::ButtonUp;
    upEv.button = platform::MouseButton::Left;
    upEv.physicalPosition = {24.0f, 16.0f};
    upEv.position = upEv.physicalPosition;
    ctx.handleMouseUp(upEv);
    EXPECT_FALSE(secPtr->isExpanded());

    WidgetKeyEvent spaceEv{platform::Key::Space, true, platform::KeyModifier::None};
    secPtr->onKeyDown(spaceEv);
    EXPECT_FALSE(spaceEv.isAccepted());
    EXPECT_FALSE(secPtr->isExpanded());

    // Hover moves while disabled must not mark hovered or draw a hover pill
    ctx.handleMouseMove(moveEv);
    renderer::PaintContext disabledPaint;
    secPtr->paint(disabledPaint);
    int boxCount = 0;
    for (const auto& cmd : disabledPaint.commands()) {
        if (std::holds_alternative<renderer::DrawBoxCommand>(cmd)) {
            ++boxCount;
        }
    }
    EXPECT_EQ(boxCount, 0);
}

TEST(CollapsibleSectionTest, ContextAttachmentSyncsDpiAndAccessoryLayoutWithoutPaint) {
    WidgetContext ctx;
    ctx.setDpiScale(1.5f);

    auto sec = std::make_unique<CollapsibleSection>("Network", false);
    auto* secPtr = sec.get();
    auto badge = std::make_unique<Label>("4");
    auto* badgePtr = secPtr->setHeaderAccessory(std::move(badge));

    // Positioned immediately when attached/set without requiring paint() first
    float initialRight = badgePtr->style().position.right.value;
    EXPECT_GT(initialRight, 0.0f);

    ctx.setRootWidget(std::move(sec));
    EXPECT_FLOAT_EQ(secPtr->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(secPtr->headerHeight(), 38.0f * 1.5f);
    EXPECT_GT(badgePtr->style().position.right.value, initialRight);

    renderer::PaintContext pctx;
    ctx.render(pctx, {320.0f, 160.0f});
    core::Rect badgeRect = badgePtr->screenRect();
    EXPECT_GE(badgeRect.origin.y, 0.0f);
    EXPECT_LE(badgeRect.origin.y + badgeRect.size.height, secPtr->headerHeight());
}

TEST(AccordionTest, FlushDividersAndVisibleTrailingSectionGuard) {
    auto acc =
        std::make_unique<Accordion>(false, SectionHeaderSize::Standard, AccordionStyle::Flush);
    auto* s1 = acc->addSection("First", nullptr, false);
    auto* s2 = acc->addSection("Second", nullptr, false);
    auto* s3 = acc->addSection("Third", nullptr, false);
    (void)s1;
    (void)s2;

    acc->layoutNode().computeLayout(320.0f, 200.0f);
    renderer::PaintContext flushAllPaint;
    acc->paint(flushAllPaint);

    int flushLineCount = 0;
    for (const auto& cmd : flushAllPaint.commands()) {
        if (std::holds_alternative<renderer::DrawLineCommand>(cmd)) {
            ++flushLineCount;
        }
    }
    EXPECT_EQ(flushLineCount, 2);

    // Hiding the trailing section leaves only one divider between S1 and S2
    s3->setVisible(false);
    acc->layoutNode().computeLayout(320.0f, 200.0f);
    renderer::PaintContext flushHiddenPaint;
    acc->paint(flushHiddenPaint);

    int hiddenLineCount = 0;
    for (const auto& cmd : flushHiddenPaint.commands()) {
        if (std::holds_alternative<renderer::DrawLineCommand>(cmd)) {
            ++hiddenLineCount;
        }
    }
    EXPECT_EQ(hiddenLineCount, 1);

    // Same trailing-hidden guard holds in Grouped mode
    acc->setAccordionStyle(AccordionStyle::Grouped);
    acc->layoutNode().computeLayout(320.0f, 200.0f);
    renderer::PaintContext groupedHiddenPaint;
    acc->paint(groupedHiddenPaint);

    int groupedLines = 0;
    for (const auto& cmd : groupedHiddenPaint.commands()) {
        if (std::holds_alternative<renderer::DrawLineCommand>(cmd)) {
            ++groupedLines;
        }
    }
    EXPECT_EQ(groupedLines, 1);
}

TEST(AccordionTest, HeaderAccessoryLabelCentersVerticallyInHeader) {
    WidgetContext ctx;
    auto secUnique = std::make_unique<CollapsibleSection>("Graphics & Rendering", true);
    CollapsibleSection* sec = secUnique.get();
    auto badge = std::make_unique<Label>("v1.3", theme::TypographyStyle::Caption,
                                         theme::TextRole::Secondary);
    Label* badgePtr = badge.get();
    sec->setHeaderAccessory(std::move(badge));

    ctx.setRootWidget(std::move(secUnique));
    ctx.handleResize(core::Size{400.0f, 160.0f});

    EXPECT_EQ(badgePtr->verticalAlignment(), VerticalAlignment::Center);
    EXPECT_FLOAT_EQ(badgePtr->layoutRect().origin.y, 0.0f);
    EXPECT_FLOAT_EQ(badgePtr->layoutRect().size.height, sec->headerHeight());
}
