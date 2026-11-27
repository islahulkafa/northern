#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/views/header_view.hpp"
#include <gtest/gtest.h>

using namespace northern::widgets;

TEST(HeaderViewTest, SectionManagementAndWidths) {
    HeaderView header;
    EXPECT_EQ(header.sectionCount(), 0u);

    header.addSection("Col 1", 100.0f, 40.0f);
    header.addSection("Col 2", 150.0f, 50.0f);
    EXPECT_EQ(header.sectionCount(), 2u);
    EXPECT_EQ(header.sectionTitle(0), "Col 1");
    EXPECT_EQ(header.sectionTitle(1), "Col 2");

    header.setStretchLastSection(false);
    EXPECT_FLOAT_EQ(header.sectionWidth(0), 100.0f);
    EXPECT_FLOAT_EQ(header.sectionWidth(1), 150.0f);
    EXPECT_FLOAT_EQ(header.totalWidth(), 250.0f);

    EXPECT_FLOAT_EQ(header.sectionPosition(0), 0.0f);
    EXPECT_FLOAT_EQ(header.sectionPosition(1), 100.0f);

    // Clamping min width
    header.setSectionWidth(0, 20.0f);
    EXPECT_FLOAT_EQ(header.sectionWidth(0), 40.0f);

    header.removeSection(0);
    EXPECT_EQ(header.sectionCount(), 1u);
    EXPECT_EQ(header.sectionTitle(0), "Col 2");
}

TEST(HeaderViewTest, SectionAndDividerHitTesting) {
    HeaderView header;
    header.setStretchLastSection(false);
    header.addSection("A", 100.0f, 40.0f);
    header.addSection("B", 100.0f, 40.0f);

    EXPECT_EQ(header.sectionAt(50.0f), 0);
    EXPECT_EQ(header.sectionAt(150.0f), 1);
    EXPECT_EQ(header.sectionAt(250.0f), -1);

    // Divider at x = 100 (radius is 4)
    EXPECT_EQ(header.dividerAt(100.0f), 0);
    EXPECT_EQ(header.dividerAt(102.0f), 0);
    EXPECT_EQ(header.dividerAt(50.0f), -1);
}

TEST(HeaderViewTest, SortOrderCycling) {
    HeaderView header;
    header.addSection("Sortable", 100.0f);
    header.addSection("Other", 100.0f);

    EXPECT_EQ(header.sectionSortOrder(0), SortOrder::None);
    EXPECT_EQ(header.sortedSection(), -1);

    int reportedSec = -1;
    SortOrder reportedOrder = SortOrder::None;
    header.onSortChanged.connect([&](size_t sec, SortOrder order) {
        reportedSec = static_cast<int>(sec);
        reportedOrder = order;
    });

    header.setSectionSortOrder(0, SortOrder::Ascending);
    EXPECT_EQ(header.sectionSortOrder(0), SortOrder::Ascending);
    EXPECT_EQ(header.sortedSection(), 0);
    EXPECT_EQ(reportedSec, 0);
    EXPECT_EQ(reportedOrder, SortOrder::Ascending);

    // Setting sort on section 1 resets section 0
    header.setSectionSortOrder(1, SortOrder::Descending);
    EXPECT_EQ(header.sectionSortOrder(0), SortOrder::None);
    EXPECT_EQ(header.sectionSortOrder(1), SortOrder::Descending);
    EXPECT_EQ(header.sortedSection(), 1);
}

TEST(HeaderViewTest, HorizontalOffsetAndMouseInteraction) {
    HeaderView header;
    header.setStretchLastSection(false);
    header.addSection("Col 0", 100.0f, 40.0f);
    header.addSection("Col 1", 100.0f, 40.0f);

    EXPECT_FLOAT_EQ(header.horizontalOffset(), 0.0f);
    header.setHorizontalOffset(50.0f);
    EXPECT_FLOAT_EQ(header.horizontalOffset(), 50.0f);

    // Negative offset clamps to 0
    header.setHorizontalOffset(-10.0f);
    EXPECT_FLOAT_EQ(header.horizontalOffset(), 0.0f);

    header.layoutNode().setLayoutRect(northern::core::Rect{northern::core::Point{0.0f, 0.0f},
                                                           northern::core::Size{200.0f, 28.0f}});
    northern::widgets::WidgetMouseEvent moveEv{northern::core::Point{100.0f, 14.0f},
                                               northern::platform::MouseButton::None,
                                               northern::platform::KeyModifier::None};
    header.onMouseMove(moveEv);
    EXPECT_EQ(header.cursorShape(), northern::platform::CursorShape::ResizeEW);

    northern::renderer::PaintContext paintCtx;
    header.paint(paintCtx);
    EXPECT_FALSE(paintCtx.commands().empty());
}

TEST(HeaderViewTest, ContextDpiAdoptionMouseGrabCancelAndThemeRefresh) {
    WidgetContext ctx;
    ctx.setDpiScale(1.5f);
    ctx.setTheme(northern::theme::Theme::dark());

    auto owned = std::make_unique<HeaderView>();
    HeaderView* header = owned.get();
    header->setStretchLastSection(false);
    header->addSection("Very Long Column Header Title", 120.0f, 40.0f);
    header->addSection("Status", 100.0f, 40.0f);
    ctx.setRootWidget(std::move(owned));
    ctx.handleResize(northern::core::Size{320.0f, 42.0f});

    EXPECT_FLOAT_EQ(header->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(header->style().height.value, HeaderView::kDefaultHeaderHeight * 1.5f);

    // Press section 0 acquires mouse grab; dragging off and releasing outside cancels sort.
    WidgetMouseEvent downSec{northern::core::Point{20.0f, 14.0f},
                             northern::platform::MouseButton::Left,
                             northern::platform::KeyModifier::None};
    header->onMouseDown(downSec);
    EXPECT_EQ(ctx.mouseGrabWidget(), header);

    header->onMouseLeave();
    EXPECT_EQ(ctx.mouseGrabWidget(), header);

    WidgetMouseEvent outsideUp{northern::core::Point{300.0f, 14.0f},
                               northern::platform::MouseButton::Left,
                               northern::platform::KeyModifier::None};
    header->onMouseUp(outsideUp);
    EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);
    EXPECT_EQ(header->sectionSortOrder(0), SortOrder::None);

    // Divider drag grab cancels cleanly on disable.
    WidgetMouseEvent downDiv{northern::core::Point{120.0f, 14.0f},
                             northern::platform::MouseButton::Left,
                             northern::platform::KeyModifier::None};
    header->onMouseDown(downDiv);
    EXPECT_EQ(ctx.mouseGrabWidget(), header);
    header->setEnabled(false);
    EXPECT_EQ(ctx.mouseGrabWidget(), nullptr);

    header->setEnabled(true);
    ctx.setTheme(northern::theme::Theme::light());
    northern::renderer::PaintContext pctx;
    header->paint(pctx);
    EXPECT_FALSE(pctx.commands().empty());
}
