#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/avatar.hpp"
#include "widgets/controls/badge.hpp"
#include "widgets/controls/image_view.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace northern;
using namespace northern::widgets;
using namespace northern::renderer;

TEST(MediaWidgetsTest, ImageViewFitModesAndPlaceholder) {
    core::Rect bounds{10.0f, 20.0f, 100.0f, 100.0f};

    // 200x100 landscape image inside 100x100 box
    auto containGeom = ImageView::computeImageLayout(bounds, 200.0f, 100.0f, ImageFit::Contain);
    EXPECT_FLOAT_EQ(containGeom.drawRect.size.width, 100.0f);
    EXPECT_FLOAT_EQ(containGeom.drawRect.size.height, 50.0f);
    EXPECT_FLOAT_EQ(containGeom.drawRect.origin.y, 45.0f);
    EXPECT_FLOAT_EQ(containGeom.uvRect.size.width, 1.0f);

    auto coverGeom = ImageView::computeImageLayout(bounds, 200.0f, 100.0f, ImageFit::Cover);
    EXPECT_FLOAT_EQ(coverGeom.drawRect.size.width, 100.0f);
    EXPECT_FLOAT_EQ(coverGeom.drawRect.size.height, 100.0f);
    EXPECT_FLOAT_EQ(coverGeom.uvRect.origin.x, 0.25f);
    EXPECT_FLOAT_EQ(coverGeom.uvRect.size.width, 0.5f);
    EXPECT_FLOAT_EQ(coverGeom.uvRect.size.height, 1.0f);

    // ScaleDown does not upscale a 40x20 image inside 100x100 box
    auto scaleDownGeom = ImageView::computeImageLayout(bounds, 40.0f, 20.0f, ImageFit::ScaleDown);
    EXPECT_FLOAT_EQ(scaleDownGeom.drawRect.size.width, 40.0f);
    EXPECT_FLOAT_EQ(scaleDownGeom.drawRect.size.height, 20.0f);

    // Placeholder paint when image is null
    WidgetContext ctx;
    auto view = std::make_unique<ImageView>();
    ImageView* viewPtr = view.get();
    viewPtr->style().width = layout::Length::points(120.0f);
    viewPtr->style().height = layout::Length::points(80.0f);
    ctx.setRootWidget(std::move(view));
    ctx.handleResize(core::Size{120.0f, 80.0f});

    PaintContext placeholderPaint;
    viewPtr->paint(placeholderPaint);
    EXPECT_FALSE(placeholderPaint.commands().empty());

    // Image paint emits DrawImageCommand
    std::vector<uint8_t> px(8 * 4 * 4, 200);
    auto img = Image::fromPixels(8, 4, px).value();
    viewPtr->setImage(img);
    PaintContext imgPaint;
    viewPtr->paint(imgPaint);
    bool hasDrawImage = false;
    for (const auto& cmd : imgPaint.commands()) {
        if (std::holds_alternative<DrawImageCommand>(cmd)) {
            hasDrawImage = true;
        }
    }
    EXPECT_TRUE(hasDrawImage);
}

TEST(MediaWidgetsTest, AvatarInitialsExtractionSizesAndStatus) {
    EXPECT_EQ(Avatar::extractInitials("Maya Lin"), "ML");
    EXPECT_EQ(Avatar::extractInitials("  liam   carter  "), "LC");
    EXPECT_EQ(Avatar::extractInitials("Alice"), "AL");
    EXPECT_EQ(Avatar::extractInitials("   "), "");

    WidgetContext ctx;
    ctx.setDpiScale(1.5f);

    auto avatar = std::make_unique<Avatar>("Maya Lin", AvatarSize::Large, AvatarShape::Circle);
    Avatar* avPtr = avatar.get();
    avPtr->setStatus(AvatarStatus::Online);
    ctx.setRootWidget(std::move(avatar));
    ctx.handleResize(core::Size{200.0f, 200.0f});

    EXPECT_FLOAT_EQ(avPtr->dpiScale(), 1.5f);
    EXPECT_FLOAT_EQ(avPtr->diameter(), 60.0f);

    PaintContext paintCtx;
    avPtr->paint(paintCtx);
    EXPECT_GE(paintCtx.commands().size(), 3u);

    std::vector<uint8_t> px(16 * 16 * 4, 180);
    avPtr->setImage(Image::fromPixels(16, 16, px).value());
    PaintContext imgPaint;
    avPtr->paint(imgPaint);
    bool hasDrawImage = false;
    for (const auto& cmd : imgPaint.commands()) {
        if (std::holds_alternative<DrawImageCommand>(cmd)) {
            hasDrawImage = true;
        }
    }
    EXPECT_TRUE(hasDrawImage);
}

TEST(MediaWidgetsTest, BadgeVariantsRolesDismissInteraction) {
    WidgetContext ctx;
    auto badge = std::make_unique<Badge>("Release v2.4", BadgeRole::Accent, BadgeVariant::Subtle);
    Badge* badgePtr = badge.get();
    badgePtr->setShowDot(true);
    badgePtr->setDismissible(true);

    int dismissedCount = 0;
    badgePtr->onDismissed.connect([&dismissedCount]() { ++dismissedCount; });

    ctx.setRootWidget(std::move(badge));
    ctx.handleResize(core::Size{200.0f, 40.0f});

    core::Rect rect = badgePtr->screenRect();
    EXPECT_FLOAT_EQ(rect.size.height, 24.0f);
    EXPECT_GT(rect.size.width, 40.0f);

    // Click the trailing close button
    core::Point closeCenter{rect.origin.x + rect.size.width - 12.0f,
                            rect.origin.y + rect.size.height * 0.5f};
    platform::MouseEvent downEv{};
    downEv.type = platform::MouseEventType::ButtonDown;
    downEv.button = platform::MouseButton::Left;
    downEv.physicalPosition = closeCenter;
    downEv.position = closeCenter;
    ctx.handleMouseDown(downEv);

    platform::MouseEvent upEv{};
    upEv.type = platform::MouseEventType::ButtonUp;
    upEv.button = platform::MouseButton::Left;
    upEv.physicalPosition = closeCenter;
    upEv.position = closeCenter;
    ctx.handleMouseUp(upEv);
    EXPECT_EQ(dismissedCount, 1);

    // Keyboard Delete dismisses when focused
    WidgetKeyEvent delEv(platform::Key::Delete, true, platform::KeyModifier::None);
    badgePtr->onKeyDown(delEv);
    EXPECT_TRUE(delEv.isAccepted());
    EXPECT_EQ(dismissedCount, 2);
}
