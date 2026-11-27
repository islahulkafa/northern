#include "widgets/dialogs/message_box.hpp"

#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/icon.hpp"
#include "widgets/controls/icons.hpp"
#include "widgets/controls/label.hpp"

namespace northern::widgets {

namespace {

constexpr float kMessageIconSize = 24.0f;

std::pair<IconId, IconRole> resolveIconSpec(MessageBoxIcon type) noexcept {
    switch (type) {
    case MessageBoxIcon::Warning:
        return {IconId::DialogWarning, IconRole::Warning};
    case MessageBoxIcon::Error:
        return {IconId::DialogError, IconRole::Danger};
    case MessageBoxIcon::Confirm:
        return {IconId::DialogConfirm, IconRole::Accent};
    case MessageBoxIcon::Info:
    default:
        return {IconId::DialogInfo, IconRole::Info};
    }
}

} // namespace

MessageBox::MessageBox(std::string title, std::string message, MessageBoxIcon iconType,
                       DialogButton buttons, std::shared_ptr<text::Font> font)
    : Dialog(std::move(title), std::move(font)), iconType_(iconType), message_(std::move(message)) {
    setCloseButtonVisible(false);
    auto row = std::make_unique<Container>();
    row->setFlexDirection(layout::FlexDirection::Row);
    row->setAlignItems(layout::AlignItems::FlexStart);
    row->setColumnGap(12.0f * dpiScale());
    row->style().width = layout::Length::percent(100.0f);

    auto [id, role] = resolveIconSpec(iconType_);
    auto icon = std::make_unique<Icon>(id, IconSize::Large, role);
    icon->setVisible(iconType_ != MessageBoxIcon::None);
    iconWidget_ = static_cast<Icon*>(row->addChild(std::move(icon)));

    auto lbl = std::make_unique<Label>(
        message_, text::FontManager::instance().defaultFont(10.5f * dpiScale()).valueOr(nullptr));
    lbl->setRole(theme::TextRole::Secondary);
    lbl->setWordWrap(text::WordWrap::Word);
    lbl->setVerticalAlignment(VerticalAlignment::Top);
    lbl->style().flexGrow = 1.0f;
    lbl->style().flexShrink = 1.0f;
    messageLabel_ = static_cast<Label*>(row->addChild(std::move(lbl)));

    setContent(std::move(row));
    setStandardButtons(buttons);
}

void MessageBox::setIconType(MessageBoxIcon iconType) {
    if (iconType_ != iconType) {
        iconType_ = iconType;
        if (iconWidget_) {
            if (iconType_ == MessageBoxIcon::None) {
                iconWidget_->setVisible(false);
            } else {
                auto [id, role] = resolveIconSpec(iconType_);
                iconWidget_->setIcon(id);
                iconWidget_->setRole(role);
                iconWidget_->setIconSize(kMessageIconSize * dpiScale());
                iconWidget_->setVisible(true);
            }
        }
        markLayoutDirty();
        markPaintDirty();
    }
}

void MessageBox::setMessage(std::string message) {
    if (message_ != message) {
        message_ = std::move(message);
        if (messageLabel_) {
            messageLabel_->setText(message_);
        }
        markLayoutDirty();
        markPaintDirty();
    }
}

void MessageBox::onDpiScaleChanged(float scale) {
    if (contentContainer() && contentContainer()->childCount() > 0) {
        if (auto* row = dynamic_cast<Container*>(contentContainer()->childAt(0))) {
            row->setColumnGap(12.0f * scale);
        }
    }
    if (iconWidget_) {
        iconWidget_->setIconSize(kMessageIconSize * scale);
    }
    if (messageLabel_) {
        messageLabel_->setDpiScale(scale);
        if (!font()) {
            messageLabel_->setFont(
                text::FontManager::instance().defaultFont(10.5f * scale).valueOr(nullptr));
        }
    }
}

void MessageBox::onFontChanged(const std::shared_ptr<text::Font>& newFont) {
    if (messageLabel_) {
        if (newFont) {
            messageLabel_->setFont(newFont);
        } else {
            messageLabel_->setFont(
                text::FontManager::instance().defaultFont(10.5f * dpiScale()).valueOr(nullptr));
        }
    }
}

MessageBox* MessageBox::showStandard(WidgetContext& ctx, std::string title, std::string message,
                                     MessageBoxIcon iconType, DialogButton buttons,
                                     std::function<void(DialogResult)> onFinishedCb,
                                     float dpiScale) {
    auto box =
        std::make_unique<MessageBox>(std::move(title), std::move(message), iconType, buttons);
    if (dpiScale > 0.0f && dpiScale != 1.0f) {
        box->setDpiScale(dpiScale);
    }
    if (onFinishedCb) {
        box->onFinished.connect(std::move(onFinishedCb));
    }
    return static_cast<MessageBox*>(Dialog::show(ctx, std::move(box)));
}

MessageBox* MessageBox::info(WidgetContext& ctx, std::string title, std::string message,
                             std::function<void(DialogResult)> onFinishedCb, float dpiScale) {
    return showStandard(ctx, std::move(title), std::move(message), MessageBoxIcon::None,
                        DialogButton::Ok, std::move(onFinishedCb), dpiScale);
}

MessageBox* MessageBox::warning(WidgetContext& ctx, std::string title, std::string message,
                                std::function<void(DialogResult)> onFinishedCb, float dpiScale) {
    return showStandard(ctx, std::move(title), std::move(message), MessageBoxIcon::Warning,
                        DialogButton::Ok, std::move(onFinishedCb), dpiScale);
}

MessageBox* MessageBox::error(WidgetContext& ctx, std::string title, std::string message,
                              std::function<void(DialogResult)> onFinishedCb, float dpiScale) {
    return showStandard(ctx, std::move(title), std::move(message), MessageBoxIcon::Error,
                        DialogButton::Ok, std::move(onFinishedCb), dpiScale);
}

MessageBox* MessageBox::confirm(WidgetContext& ctx, std::string title, std::string message,
                                std::function<void(DialogResult)> onFinishedCb, float dpiScale) {
    return showStandard(ctx, std::move(title), std::move(message), MessageBoxIcon::None,
                        DialogButton::Yes | DialogButton::No, std::move(onFinishedCb), dpiScale);
}

} // namespace northern::widgets
