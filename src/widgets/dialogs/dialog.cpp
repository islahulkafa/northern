#include "widgets/dialogs/dialog.hpp"

#include "text/font_manager.hpp"
#include "widgets/base/overlay.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/icons.hpp"
#include "widgets/controls/label.hpp"

#include <algorithm>
#include <array>

namespace northern::widgets {

namespace {

constexpr float kDefaultPadding = 16.0f;
constexpr float kDefaultGap = 6.0f;
constexpr float kFooterGap = 8.0f;

const char* standardButtonText(DialogButton btn) noexcept {
    switch (btn) {
    case DialogButton::Ok:
        return "OK";
    case DialogButton::Cancel:
        return "Cancel";
    case DialogButton::Yes:
        return "Yes";
    case DialogButton::No:
        return "No";
    case DialogButton::Apply:
        return "Apply";
    case DialogButton::Close:
        return "Close";
    default:
        return "";
    }
}

DialogResult standardButtonResult(DialogButton btn) noexcept {
    switch (btn) {
    case DialogButton::Ok:
    case DialogButton::Yes:
    case DialogButton::Apply:
        return DialogResult::Accepted;
    case DialogButton::Cancel:
    case DialogButton::No:
    case DialogButton::Close:
    default:
        return DialogResult::Rejected;
    }
}

} // namespace

Dialog::Dialog(std::string title, std::shared_ptr<text::Font> font)
    : title_(std::move(title)), font_(std::move(font)) {
    setFocusable(false);
    setSurface(SurfaceRole::Overlay);
    setBorderRole(BorderRole::Default);
    setFlexDirection(layout::FlexDirection::Column);

    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float availW, float) -> core::Size {
            float targetW = dialogWidth_ * dpiScale_;
            float w = availW > 0.0f ? std::min(targetW, availW * 0.92f) : targetW;
            return core::Size{w, 120.0f * dpiScale_};
        });

    auto header = std::make_unique<Container>();
    header->setFlexDirection(layout::FlexDirection::Row);
    header->setJustifyContent(layout::JustifyContent::SpaceBetween);
    header->setAlignItems(layout::AlignItems::Center);
    header->style().width = layout::Length::percent(100.0f);

    auto titleLbl = std::make_unique<Label>(
        title_, text::FontManager::instance().semiBoldFont(16.0f * dpiScale_).valueOr(nullptr));
    titleLbl->setRole(theme::TextRole::Primary);
    titleLbl->style().flexGrow = 1.0f;
    titleLbl->style().flexShrink = 1.0f;
    titleLbl->setOverflow(text::TextOverflow::Ellipsis);
    titleLabel_ = static_cast<Label*>(header->addChild(std::move(titleLbl)));

    auto closeBtn =
        std::make_unique<Button>(IconId::Close, ButtonVariant::Ghost, ButtonSize::Compact);
    closeBtn->onClicked.connect([this]() { reject(); });
    closeButton_ = static_cast<Button*>(header->addChild(std::move(closeBtn)));

    headerRow_ = static_cast<Container*>(addChild(std::move(header)));

    auto content = std::make_unique<Container>();
    content->setFlexDirection(layout::FlexDirection::Column);
    content->style().width = layout::Length::percent(100.0f);
    contentContainer_ = static_cast<Container*>(addChild(std::move(content)));

    auto footer = std::make_unique<Container>();
    footer->setFlexDirection(layout::FlexDirection::Row);
    footer->setJustifyContent(layout::JustifyContent::FlexEnd);
    footer->setAlignItems(layout::AlignItems::Center);
    footer->style().width = layout::Length::percent(100.0f);
    footerRow_ = static_cast<Container*>(addChild(std::move(footer)));
    footerRow_->setVisible(false);

    applyMetrics();
}

Dialog::~Dialog() {
    if (isOpen_ && context() && !isClosing_) {
        isClosing_ = true;
        isOpen_ = false;
        context()->dismissOverlaysForOwner(this);
    }
}

Dialog* Dialog::show(WidgetContext& ctx, std::unique_ptr<Dialog> dialog) {
    if (!dialog) {
        return nullptr;
    }
    Dialog* raw = dialog.get();
    raw->configureOverlayAndShow(ctx, true, std::move(dialog));
    return raw;
}

void Dialog::open(WidgetContext& ctx) {
    if (isOpen_) {
        return;
    }
    configureOverlayAndShow(ctx, false, nullptr);
}

void Dialog::configureOverlayAndShow(WidgetContext& ctx, bool takeOwnership,
                                     std::unique_ptr<Dialog> ownedPtr) {
    isClosing_ = false;
    isOpen_ = true;
    result_ = DialogResult::Rejected;

    OverlayDesc desc{};
    desc.preferredPlacement = OverlayPlacement::Center;
    desc.autoFlip = false;
    desc.dismissOnOutsideClick = dismissOnOutsideClick_;
    desc.dismissOnEscape = dismissOnEscape_;
    desc.isModal = true;
    desc.gap = 0.0f;
    desc.onDismiss = [this]() { handleExternalDismiss(); };

    if (takeOwnership) {
        ctx.showOverlay(std::move(ownedPtr), desc, this);
    } else {
        ctx.showOverlay(this, desc, this);
    }

    ctx.setFocusedWidget(this, FocusReason::Programmatic);
}

void Dialog::handleExternalDismiss() {
    isOpen_ = false;
    if (isClosing_) {
        return;
    }
    isClosing_ = true;
    result_ = DialogResult::Rejected;
    onRejected.emit();
    onFinished.emit(DialogResult::Rejected);
}

void Dialog::accept() {
    close(DialogResult::Accepted);
}

void Dialog::reject() {
    close(DialogResult::Rejected);
}

void Dialog::close(DialogResult res) {
    if (isClosing_) {
        return;
    }
    isClosing_ = true;
    isOpen_ = false;
    result_ = res;

    if (res == DialogResult::Accepted) {
        onAccepted.emit();
    } else if (res == DialogResult::Rejected) {
        onRejected.emit();
    }
    onFinished.emit(res);

    WidgetContext* ctx = context();
    if (ctx) {
        ctx->dismissOverlaysForOwner(this);
    }
}

void Dialog::setTitle(std::string title) {
    if (title_ != title) {
        title_ = std::move(title);
        if (titleLabel_) {
            titleLabel_->setText(title_);
        }
        markLayoutDirty();
        markPaintDirty();
    }
}

void Dialog::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        for (auto& [btnMask, btnPtr] : buttonEntries_) {
            (void)btnMask;
            if (btnPtr) {
                btnPtr->setFont(font_);
            }
        }
        onFontChanged(font_);
        markLayoutDirty();
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> Dialog::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float px = 14.0f * dpiScale_;
        resolvedDefaultFont_ = text::FontManager::instance().defaultFont(px).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void Dialog::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void Dialog::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        applyMetrics();
        onDpiScaleChanged(clamped);
        markLayoutDirty();
        markPaintDirty();
    }
}

void Dialog::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void Dialog::onThemeChanged() {
    resolvedDefaultFont_.reset();
    Container::onThemeChanged();
    markLayoutDirty();
    markPaintDirty();
}

void Dialog::onDpiScaleChanged(float) {}

void Dialog::onFontChanged(const std::shared_ptr<text::Font>&) {}

void Dialog::setDialogWidth(float width) {
    float clamped = std::max(240.0f, width);
    if (dialogWidth_ != clamped) {
        dialogWidth_ = clamped;
        applyMetrics();
        markLayoutDirty();
    }
}

void Dialog::setCloseButtonVisible(bool visible) {
    if (closeButtonVisible_ != visible) {
        closeButtonVisible_ = visible;
        if (closeButton_) {
            closeButton_->setVisible(visible);
        }
        markLayoutDirty();
    }
}

void Dialog::setContent(std::unique_ptr<Widget> content) {
    if (!contentContainer_) {
        return;
    }
    while (contentContainer_->childCount() > 0) {
        contentContainer_->removeChild(contentContainer_->childAt(0));
    }
    if (content) {
        contentContainer_->addChild(std::move(content));
    }
    markLayoutDirty();
    markPaintDirty();
}

void Dialog::setStandardButtons(DialogButton buttons) {
    standardButtons_ = buttons;
    buttonEntries_.clear();
    defaultButton_ = nullptr;

    if (footerRow_) {
        while (footerRow_->childCount() > 0) {
            footerRow_->removeChild(footerRow_->childAt(0));
        }
    }

    const bool hasPrimaryOk = hasDialogButton(buttons, DialogButton::Ok);
    static constexpr std::array<DialogButton, 6> kOrder{
        DialogButton::Apply, DialogButton::Ok,     DialogButton::Yes,
        DialogButton::No,    DialogButton::Cancel, DialogButton::Close,
    };

    for (DialogButton b : kOrder) {
        if (!hasDialogButton(buttons, b)) {
            continue;
        }
        bool isPrimary = (b == DialogButton::Ok) || (!hasPrimaryOk && b == DialogButton::Yes);
        ButtonVariant variant = isPrimary ? ButtonVariant::Primary : ButtonVariant::Secondary;
        DialogResult res = standardButtonResult(b);

        auto btn = std::make_unique<Button>(standardButtonText(b), font_, variant);
        btn->setDpiScale(dpiScale_);
        btn->onClicked.connect([this, res]() { close(res); });

        Button* rawBtn = static_cast<Button*>(footerRow_->addChild(std::move(btn)));
        buttonEntries_.emplace_back(b, rawBtn);
        if (isPrimary && !defaultButton_) {
            defaultButton_ = rawBtn;
        }
    }

    if (footerRow_) {
        footerRow_->setVisible(footerRow_->childCount() > 0);
    }
    markLayoutDirty();
    markPaintDirty();
}

Button* Dialog::button(DialogButton which) const noexcept {
    for (const auto& [flag, ptr] : buttonEntries_) {
        if (flag == which) {
            return ptr;
        }
    }
    return nullptr;
}

Button* Dialog::addButton(std::string text, ButtonVariant variant, DialogResult result) {
    auto btn = std::make_unique<Button>(std::move(text), font_, variant);
    btn->setDpiScale(dpiScale_);
    btn->onClicked.connect([this, result]() { close(result); });
    Button* rawBtn = static_cast<Button*>(footerRow_->addChild(std::move(btn)));
    buttonEntries_.emplace_back(DialogButton::None, rawBtn);
    if (variant == ButtonVariant::Primary && !defaultButton_) {
        defaultButton_ = rawBtn;
    }
    footerRow_->setVisible(true);
    markLayoutDirty();
    markPaintDirty();
    return rawBtn;
}

void Dialog::onKeyDown(WidgetKeyEvent& ev) {
    if (!ev.isDown()) {
        Container::onKeyDown(ev);
        return;
    }
    if (ev.key() == platform::Key::Escape && dismissOnEscape_) {
        ev.accept();
        reject();
        return;
    }
    if (ev.key() == platform::Key::Enter) {
        ev.accept();
        if (defaultButton_) {
            defaultButton_->onClicked.emit();
        } else {
            accept();
        }
        return;
    }
    Container::onKeyDown(ev);
}

void Dialog::applyMetrics() {
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    setCornerRadius(theme::Metrics::CornerRadiusOverlay * scale);
    setPadding(kDefaultPadding * scale);
    setRowGap(kDefaultGap * scale);
    style().width = layout::Length::points(dialogWidth_ * scale);
    style().minWidth = layout::Length::points(280.0f * scale);

    if (titleLabel_) {
        titleLabel_->setDpiScale(scale);
        if (!font_) {
            titleLabel_->setFont(
                text::FontManager::instance().semiBoldFont(16.0f * scale).valueOr(nullptr));
        }
    }
    if (closeButton_) {
        closeButton_->setDpiScale(scale);
    }
    if (contentContainer_) {
        contentContainer_->setRowGap(4.0f * scale);
    }
    if (footerRow_) {
        footerRow_->setColumnGap(kFooterGap * scale);
        footerRow_->style().margin.top = layout::Length::points(4.0f * scale);
    }
    for (auto& [btnMask, btnPtr] : buttonEntries_) {
        (void)btnMask;
        if (btnPtr) {
            btnPtr->setDpiScale(scale);
        }
    }
}

} // namespace northern::widgets
