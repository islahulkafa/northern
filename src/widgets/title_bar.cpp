#include "widgets/title_bar.hpp"
#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/container.hpp"
#include "widgets/theme.hpp"

#include <algorithm>

namespace northern::widgets {

TitleBar::TitleBar(std::string title, std::shared_ptr<text::Font> font, platform::Window* window)
    : title_(std::move(title)), font_(std::move(font)) {
    style().width = layout::Length::percent(100.0f);
    style().height = layout::Length::points(kDefaultHeight);
    style().flexDirection = layout::FlexDirection::Row;
    style().alignItems = layout::AlignItems::Center;
    style().justifyContent = layout::JustifyContent::Center;

    appLogo_ = std::make_unique<Icon>(IconId::AppLogo, 18.0f, theme::ColorTokens::AccentDefault);
    minIcon_ =
        std::make_unique<Icon>(IconId::WindowMinimize, 14.0f, theme::ColorTokens::TextSecondary);
    maxIcon_ =
        std::make_unique<Icon>(IconId::WindowMaximize, 14.0f, theme::ColorTokens::TextSecondary);
    restoreIcon_ =
        std::make_unique<Icon>(IconId::WindowRestore, 14.0f, theme::ColorTokens::TextSecondary);
    closeIcon_ =
        std::make_unique<Icon>(IconId::WindowClose, 14.0f, theme::ColorTokens::TextSecondary);

    if (window) {
        setWindow(window);
    }
}

void TitleBar::setWindow(platform::Window* window) {
    window_ = window;
    if (window_) {
        window_->setHitTestCallback(
            [this](core::Point screenPos) { return hitTestPoint(screenPos); });
        window_->onResize.connect([this](const auto&) { updateWindowState(); });
        updateWindowState();
    }
}

void TitleBar::setTitle(std::string title) {
    if (title_ != title) {
        title_ = std::move(title);
        markPaintDirty();
    }
}

void TitleBar::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        markPaintDirty();
    }
}

void TitleBar::setCenterWidget(std::unique_ptr<Widget> widget) {
    if (centerWidget_) {
        removeChild(centerWidget_);
        centerWidget_ = nullptr;
    }
    if (widget) {
        centerWidget_ = addChild(std::move(widget));
    }
    markLayoutDirty();
}

void TitleBar::addActionWidget(std::unique_ptr<Widget> widget) {
    if (widget) {
        actionWidgets_.push_back(std::unique_ptr<Widget>(addChild(std::move(widget))));
        markLayoutDirty();
    }
}

void TitleBar::updateWindowState() {
    if (window_) {
        bool maximized = window_->isMaximized();
        if (isMaximized_ != maximized) {
            isMaximized_ = maximized;
            markPaintDirty();
        }
    }
}

TitleBar::CaptionButton TitleBar::hitTestCaptionButton(core::Point localPos) const {
    float width = layoutRect().size.width;
    float height = layoutRect().size.height > 0.0f ? layoutRect().size.height : kDefaultHeight;

    if (localPos.y < 0.0f || localPos.y > height) {
        return CaptionButton::None;
    }

    float closeLeft = width - kCaptionButtonWidth;
    float maxLeft = closeLeft - kCaptionButtonWidth;
    float minLeft = maxLeft - kCaptionButtonWidth;

    if (localPos.x >= closeLeft && localPos.x <= width) {
        return CaptionButton::Close;
    }
    if (localPos.x >= maxLeft && localPos.x < closeLeft) {
        return CaptionButton::Maximize;
    }
    if (localPos.x >= minLeft && localPos.x < maxLeft) {
        return CaptionButton::Minimize;
    }
    return CaptionButton::None;
}

core::Rect TitleBar::captionButtonRect(CaptionButton btn) const {
    core::Rect sRect = screenRect();
    float height = sRect.size.height > 0.0f ? sRect.size.height : kDefaultHeight;
    float width = sRect.size.width;

    float closeLeft = sRect.origin.x + width - kCaptionButtonWidth;
    float maxLeft = closeLeft - kCaptionButtonWidth;
    float minLeft = maxLeft - kCaptionButtonWidth;

    switch (btn) {
    case CaptionButton::Close:
        return core::Rect{core::Point{closeLeft, sRect.origin.y},
                          core::Size{kCaptionButtonWidth, height}};
    case CaptionButton::Maximize:
        return core::Rect{core::Point{maxLeft, sRect.origin.y},
                          core::Size{kCaptionButtonWidth, height}};
    case CaptionButton::Minimize:
        return core::Rect{core::Point{minLeft, sRect.origin.y},
                          core::Size{kCaptionButtonWidth, height}};
    default:
        return core::Rect{};
    }
}

platform::HitTestResult TitleBar::hitTestPoint(core::Point screenPos) const {
    core::Rect sRect = screenRect();
    if (!sRect.contains(screenPos)) {
        return platform::HitTestResult::Client;
    }

    core::Point localPos{screenPos.x - sRect.origin.x, screenPos.y - sRect.origin.y};
    CaptionButton btn = hitTestCaptionButton(localPos);
    if (btn != CaptionButton::None) {
        return platform::HitTestResult::Client;
    }

    if (centerWidget_ && centerWidget_->screenRect().contains(screenPos)) {
        return platform::HitTestResult::Client;
    }

    for (const auto& action : actionWidgets_) {
        if (action && action->screenRect().contains(screenPos)) {
            return platform::HitTestResult::Client;
        }
    }

    return platform::HitTestResult::Caption;
}

void TitleBar::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    float height = sRect.size.height > 0.0f ? sRect.size.height : kDefaultHeight;

    ctx.drawRect(sRect, theme::ColorTokens::SurfaceDefault);
    ctx.drawLine(core::Point{sRect.origin.x, sRect.origin.y + height - 0.5f},
                 core::Point{sRect.origin.x + sRect.size.width, sRect.origin.y + height - 0.5f},
                 theme::ColorTokens::BorderSubtle, 1.0f);

    if (appLogo_) {
        core::Rect logoRect{
            core::Point{sRect.origin.x + 12.0f, sRect.origin.y + (height - 18.0f) * 0.5f},
            core::Size{18.0f, 18.0f}};
        if (auto doc = appLogo_->svgDocument()) {
            ctx.drawSvg(*doc, logoRect, theme::ColorTokens::AccentDefault);
        }
    }

    if (font_ && !title_.empty()) {
        float titleX = sRect.origin.x + 36.0f;
        float titleY = theme::computeCenteredTextY(sRect.origin.y + height * 0.5f, *font_);
        auto layout = text::TextLayout::create(title_, font_);
        ctx.drawTextLayout(core::Point{titleX, titleY}, layout, theme::ColorTokens::TextPrimary);
    }

    auto paintBtn = [&](CaptionButton btn, Icon* icon, bool isClose) {
        core::Rect btnRect = captionButtonRect(btn);
        bool isHovered = (hoveredBtn_ == btn);
        bool isPressed = (pressedBtn_ == btn);

        if (isClose) {
            if (isPressed) {
                ctx.drawRect(btnRect, core::Color::fromRgba8(196, 43, 28));
            } else if (isHovered) {
                ctx.drawRect(btnRect, core::Color::fromRgba8(232, 17, 35));
            }
        } else {
            if (isPressed) {
                ctx.drawRect(btnRect, theme::ColorTokens::SurfaceOverlay);
            } else if (isHovered) {
                ctx.drawRect(btnRect, theme::ColorTokens::SurfaceRaised);
            }
        }

        if (icon) {
            core::Color iconColor = (isClose && isHovered)
                                        ? core::Color::white()
                                        : (isHovered ? theme::ColorTokens::TextPrimary
                                                     : theme::ColorTokens::TextSecondary);
            core::Rect iconRect{
                core::Point{btnRect.origin.x + (btnRect.size.width - 14.0f) * 0.5f,
                            btnRect.origin.y + (btnRect.size.height - 14.0f) * 0.5f},
                core::Size{14.0f, 14.0f}};
            if (auto doc = icon->svgDocument()) {
                ctx.drawSvg(*doc, iconRect, iconColor);
            }
        }
    };

    paintBtn(CaptionButton::Minimize, minIcon_.get(), false);
    paintBtn(CaptionButton::Maximize, isMaximized_ ? restoreIcon_.get() : maxIcon_.get(), false);
    paintBtn(CaptionButton::Close, closeIcon_.get(), true);
}

void TitleBar::onMouseEnter() {}

void TitleBar::onMouseLeave() {
    if (hoveredBtn_ != CaptionButton::None || pressedBtn_ != CaptionButton::None) {
        hoveredBtn_ = CaptionButton::None;
        pressedBtn_ = CaptionButton::None;
        markPaintDirty();
    }
}

void TitleBar::onMouseMove(WidgetMouseEvent& ev) {
    CaptionButton btn = hitTestCaptionButton(ev.position());
    if (hoveredBtn_ != btn) {
        hoveredBtn_ = btn;
        markPaintDirty();
    }
}

void TitleBar::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        CaptionButton btn = hitTestCaptionButton(ev.position());
        if (btn != CaptionButton::None) {
            pressedBtn_ = btn;
            ev.accept();
            markPaintDirty();
        }
    }
}

void TitleBar::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        CaptionButton btn = hitTestCaptionButton(ev.position());
        if (btn != CaptionButton::None && btn == pressedBtn_) {
            ev.accept();
            pressedBtn_ = CaptionButton::None;
            markPaintDirty();

            switch (btn) {
            case CaptionButton::Minimize:
                if (window_) {
                    window_->minimize();
                }
                onMinimizeClicked.emit();
                break;
            case CaptionButton::Maximize:
                if (window_) {
                    window_->toggleMaximize();
                    updateWindowState();
                }
                onMaximizeClicked.emit();
                break;
            case CaptionButton::Close:
                if (window_) {
                    window_->requestClose();
                }
                onCloseClicked.emit();
                break;
            default:
                break;
            }
        } else {
            pressedBtn_ = CaptionButton::None;
            markPaintDirty();
        }
    }
}

} // namespace northern::widgets
