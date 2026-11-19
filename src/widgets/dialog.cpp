#include "widgets/dialog.hpp"
#include "renderer/paint_context.hpp"
#include "widgets/theme.hpp"

namespace northern::widgets {

Dialog::Dialog(std::string title, std::shared_ptr<text::Font> font)
    : title_(std::move(title)), font_(std::move(font)) {
    setFocusable(true);
    style().width = layout::Length::points(420.0f);
    style().flexDirection = layout::FlexDirection::Column;
    style().padding = layout::RectLength::all(layout::Length::points(16.0f));
    style().rowGap = 16.0f;

    headerContainer_ = addChild(std::make_unique<Container>());
    headerContainer_->style().width = layout::Length::percent(100.0f);
    headerContainer_->style().flexDirection = layout::FlexDirection::Row;
    headerContainer_->style().alignItems = layout::AlignItems::Center;
    headerContainer_->style().columnGap = 10.0f;

    statusIconWidget_ = headerContainer_->addChild(
        std::make_unique<Icon>(IconId::DialogInfo, 22.0f, theme::ColorTokens::AccentDefault));
    statusIconWidget_->setVisible(false);

    titleLabel_ = headerContainer_->addChild(std::make_unique<Label>(title_, font_));
    titleLabel_->style().flexGrow = 1.0f;

    auto closeBtn = std::make_unique<Button>("", font_, ButtonVariant::Ghost);
    closeBtn->style().width = layout::Length::points(24.0f);
    closeBtn->style().height = layout::Length::points(24.0f);
    closeBtn->addChild(
        std::make_unique<Icon>(IconId::Close, 14.0f, theme::ColorTokens::TextSecondary));
    closeBtn->onClicked.connect([this]() { reject(); });
    headerContainer_->addChild(std::move(closeBtn));

    bodyContainer_ = addChild(std::make_unique<Container>());
    bodyContainer_->style().width = layout::Length::percent(100.0f);
    bodyContainer_->style().flexDirection = layout::FlexDirection::Column;
    bodyContainer_->style().rowGap = 8.0f;

    messageLabel_ = bodyContainer_->addChild(std::make_unique<Label>("", font_));
    messageLabel_->setVisible(false);

    footerContainer_ = addChild(std::make_unique<Container>());
    footerContainer_->style().width = layout::Length::percent(100.0f);
    footerContainer_->style().flexDirection = layout::FlexDirection::Row;
    footerContainer_->style().justifyContent = layout::JustifyContent::FlexEnd;
    footerContainer_->style().columnGap = 8.0f;
}

void Dialog::setTitle(std::string title) {
    if (title_ != title) {
        title_ = std::move(title);
        if (titleLabel_) {
            titleLabel_->setText(title_);
        }
        markPaintDirty();
    }
}

void Dialog::setStatusIcon(IconId iconId, core::Color color) {
    if (statusIconWidget_) {
        statusIconWidget_->setIcon(iconId);
        statusIconWidget_->setColor(color);
        statusIconWidget_->setVisible(true);
        markLayoutDirty();
    }
}

void Dialog::clearStatusIcon() {
    if (statusIconWidget_) {
        statusIconWidget_->setVisible(false);
        markLayoutDirty();
    }
}

void Dialog::setMessage(std::string message) {
    message_ = std::move(message);
    if (messageLabel_) {
        messageLabel_->setText(message_);
        messageLabel_->setVisible(!message_.empty());
    }
    markLayoutDirty();
}

void Dialog::setCustomContentInternal(std::unique_ptr<Widget> content) {
    bodyContainer_->clearChildren();
    messageLabel_ = nullptr;
    if (content) {
        bodyContainer_->addChild(std::move(content));
    }
    markLayoutDirty();
}

void Dialog::addActionButton(std::string text, ButtonVariant variant,
                             std::function<void()> onClicked) {
    auto btn = std::make_unique<Button>(std::move(text), font_, variant);
    if (onClicked) {
        btn->onClicked.connect(std::move(onClicked));
    }
    footerContainer_->addChild(std::move(btn));
    markLayoutDirty();
}

void Dialog::open(WidgetContext& ctx, bool lightDismiss) {
    activeContext_ = &ctx;
    OverlayDesc desc{};
    desc.preferredPlacement = OverlayPlacement::Center;
    desc.isModal = true;
    desc.dismissOnOutsideClick = lightDismiss;
    desc.dismissOnEscape = true;
    ctx.showOverlay(std::unique_ptr<Widget>(this), desc);
}

void Dialog::accept() {
    onAccepted.emit();
    onFinished.emit(DialogResult::Accepted);
    if (activeContext_) {
        activeContext_->dismissTopOverlay();
    }
}

void Dialog::reject() {
    onRejected.emit();
    onFinished.emit(DialogResult::Rejected);
    if (activeContext_) {
        activeContext_->dismissTopOverlay();
    }
}

void Dialog::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    ctx.drawRoundedRect(sRect, theme::ColorTokens::SurfaceRaised, theme::Metrics::CornerRadiusCard);
    ctx.drawBorder(sRect, theme::ColorTokens::BorderSubtle, 1.0f, theme::Metrics::CornerRadiusCard);
}

void Dialog::onKeyDown(WidgetKeyEvent& ev) {
    if (ev.key() == platform::Key::Escape) {
        ev.accept();
        reject();
    } else if (ev.key() == platform::Key::Enter) {
        ev.accept();
        accept();
    }
}

} // namespace northern::widgets
