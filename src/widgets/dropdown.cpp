#include "widgets/dropdown.hpp"
#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/widget_context.hpp"

#include <algorithm>

namespace northern::widgets {

class DropdownPopup : public Widget {
public:
    DropdownPopup(Dropdown* owner, std::vector<std::string> items, int selectedIndex,
                  std::shared_ptr<text::Font> font, float targetWidth)
        : owner_(owner), items_(std::move(items)), selectedIndex_(selectedIndex),
          font_(std::move(font)), targetWidth_(targetWidth) {
        setFocusable(false);
        style().width = layout::Length::points(targetWidth_);

        itemHeight_ = 28.0f;
        float paddingVertical = 8.0f;
        float contentH = static_cast<float>(items_.size()) * itemHeight_ + paddingVertical;
        float maxH = 176.0f;
        float totalH = std::min(contentH, maxH);
        style().height = layout::Length::points(totalH);

        layoutNode_.setMeasureFunc(
            [this, totalH](const layout::LayoutNode&, float, float) -> core::Size {
                return core::Size{targetWidth_, totalH};
            });
    }

    void onMouseMove(WidgetMouseEvent& ev) override {
        float localY = ev.position().y - 4.0f + scrollOffset_;
        int idx = static_cast<int>(localY / itemHeight_);
        if (idx >= 0 && idx < static_cast<int>(items_.size())) {
            if (idx != hoveredIndex_) {
                hoveredIndex_ = idx;
                markPaintDirty();
            }
        } else if (hoveredIndex_ != -1) {
            hoveredIndex_ = -1;
            markPaintDirty();
        }
        ev.accept();
    }

    void onMouseLeave() override {
        if (hoveredIndex_ != -1) {
            hoveredIndex_ = -1;
            markPaintDirty();
        }
    }

    void onMouseDown(WidgetMouseEvent& ev) override {
        if (ev.button() == platform::MouseButton::Left) {
            ev.accept();
        }
    }

    void onMouseUp(WidgetMouseEvent& ev) override {
        if (ev.button() == platform::MouseButton::Left) {
            float localY = ev.position().y - 4.0f + scrollOffset_;
            int idx = static_cast<int>(localY / itemHeight_);
            if (idx >= 0 && idx < static_cast<int>(items_.size())) {
                if (owner_) {
                    owner_->selectAndClose(idx);
                }
            }
            ev.accept();
        }
    }

    void onMouseWheel(WidgetMouseEvent& ev) override {
        float maxScroll = std::max(0.0f, static_cast<float>(items_.size()) * itemHeight_ + 8.0f -
                                             layoutRect().size.height);
        if (maxScroll > 0.0f) {
            scrollOffset_ =
                std::clamp(scrollOffset_ - ev.wheelDelta().y * itemHeight_, 0.0f, maxScroll);
            markPaintDirty();
            ev.accept();
        }
    }

    void paint(renderer::PaintContext& ctx) override {
        core::Rect rect = screenRect();
        if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
            return;
        }

        float radius = 6.0f;
        float radii[4] = {radius, radius, radius, radius};
        core::Color bg = theme::ColorTokens::SurfaceOverlay;
        core::Color border = theme::ColorTokens::BorderSubtle;

        ctx.drawBox(rect, bg, border, 1.0f, radii);
        ctx.pushClip(rect);

        float itemY = rect.origin.y + 4.0f - scrollOffset_;
        float padX = 10.0f;

        for (size_t i = 0; i < items_.size(); ++i) {
            core::Rect itemRect{core::Point{rect.origin.x + 3.0f, itemY},
                                core::Size{std::max(0.0f, rect.size.width - 6.0f), itemHeight_}};

            if (itemRect.origin.y + itemHeight_ >= rect.origin.y &&
                itemRect.origin.y <= rect.origin.y + rect.size.height) {
                bool isSelected = (static_cast<int>(i) == selectedIndex_);
                bool isHovered = (static_cast<int>(i) == hoveredIndex_);

                if (isSelected || isHovered) {
                    core::Color itemBg = isSelected ? core::Color{0.145f, 0.388f, 0.922f, 0.22f}
                                                    : core::Color{1.0f, 1.0f, 1.0f, 0.08f};
                    float itemRadii[4] = {4.0f, 4.0f, 4.0f, 4.0f};
                    ctx.drawRoundedRect(itemRect, itemBg, itemRadii);
                }

                if (font_) {
                    float targetCenterY = itemRect.origin.y + itemHeight_ * 0.5f;
                    float textY = theme::computeCenteredTextY(targetCenterY, *font_);
                    core::Color textCol = theme::ColorTokens::TextPrimary;
                    auto layout = text::TextLayout::create(items_[i], font_);
                    ctx.drawTextLayout(core::Point{itemRect.origin.x + padX, textY}, layout,
                                       textCol);
                }
            }

            itemY += itemHeight_;
        }

        ctx.popClip();
    }

private:
    Dropdown* owner_{nullptr};
    std::vector<std::string> items_{};
    int selectedIndex_{-1};
    int hoveredIndex_{-1};
    std::shared_ptr<text::Font> font_{nullptr};
    float targetWidth_{160.0f};
    float itemHeight_{28.0f};
    float scrollOffset_{0.0f};
};

Dropdown::Dropdown(std::vector<std::string> items, int selectedIndex,
                   std::shared_ptr<text::Font> font)
    : items_(std::move(items)), selectedIndex_(selectedIndex), font_(std::move(font)) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::PointingHand);
    style().height = layout::Length::points(theme::Metrics::ControlHeightStandard);
    style().width = layout::Length::percent(100.0f);

    layoutNode_.setMeasureFunc([](const layout::LayoutNode&, float availW, float) -> core::Size {
        float w = std::max(120.0f, availW);
        return core::Size{w, theme::Metrics::ControlHeightStandard};
    });
}

Dropdown::~Dropdown() {
    if (isOpen_ && context_) {
        context_->dismissOverlaysForOwner(this);
    }
}

void Dropdown::setItems(std::vector<std::string> items) {
    items_ = std::move(items);
    if (selectedIndex_ >= static_cast<int>(items_.size())) {
        selectedIndex_ = items_.empty() ? -1 : static_cast<int>(items_.size()) - 1;
    }
    markPaintDirty();
}

void Dropdown::addItem(std::string item) {
    items_.push_back(std::move(item));
    markPaintDirty();
}

void Dropdown::clearItems() {
    items_.clear();
    selectedIndex_ = -1;
    markPaintDirty();
}

void Dropdown::setSelectedIndex(int index) {
    int clamped = std::clamp(index, -1, static_cast<int>(items_.size()) - 1);
    if (selectedIndex_ != clamped) {
        selectedIndex_ = clamped;
        markPaintDirty();
        onSelectionChanged.emit(selectedIndex_, selectedText());
    }
}

std::string Dropdown::selectedText() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        return items_[selectedIndex_];
    }
    return "";
}

void Dropdown::setPlaceholder(std::string placeholder) {
    placeholder_ = std::move(placeholder);
    markPaintDirty();
}

void Dropdown::setFont(std::shared_ptr<text::Font> font) {
    font_ = std::move(font);
    markPaintDirty();
}

void Dropdown::open() {
    if (isOpen_) {
        return;
    }
    isOpen_ = true;
    if (context_) {
        core::Rect r = screenRect();
        float w = std::max(120.0f, r.size.width);
        auto popup = std::make_unique<DropdownPopup>(this, items_, selectedIndex_, font_, w);

        OverlayDesc desc{};
        desc.anchorRect = r;
        desc.preferredPlacement = OverlayPlacement::Bottom;
        desc.autoFlip = true;
        desc.dismissOnOutsideClick = true;
        desc.dismissOnEscape = true;
        desc.gap = 4.0f;
        desc.onDismiss = [this]() { onPopupDismissed(); };

        context_->showOverlay(std::move(popup), desc, this);
        context_->registerAnimatedWidget(this);
    }

    popupAnim_.start(popupAnim_.currentValue(), 1.0f, 0.15f,
                     core::animation::EasingCurve::OutCubic);
    markPaintDirty();
}

void Dropdown::close() {
    if (!isOpen_) {
        return;
    }
    isOpen_ = false;
    if (context_) {
        context_->dismissOverlaysForOwner(this);
        context_->registerAnimatedWidget(this);
    }
    popupAnim_.start(popupAnim_.currentValue(), 0.0f, 0.12f,
                     core::animation::EasingCurve::OutCubic);
    markPaintDirty();
}

void Dropdown::toggle() {
    if (isOpen_) {
        close();
    } else {
        open();
    }
}

void Dropdown::selectAndClose(int index) {
    setSelectedIndex(index);
    close();
}

void Dropdown::onPopupDismissed() {
    isOpen_ = false;
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    popupAnim_.start(popupAnim_.currentValue(), 0.0f, 0.12f,
                     core::animation::EasingCurve::OutCubic);
    markPaintDirty();
}

void Dropdown::onMouseEnter() {
    isHovered_ = true;
    markPaintDirty();
}

void Dropdown::onMouseLeave() {
    isHovered_ = false;
    markPaintDirty();
}

void Dropdown::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        setFocus();
        toggle();
        ev.accept();
    }
}

void Dropdown::onKeyDown(WidgetKeyEvent& ev) {
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        toggle();
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Down) {
        if (!isOpen_) {
            open();
        } else if (selectedIndex_ + 1 < static_cast<int>(items_.size())) {
            setSelectedIndex(selectedIndex_ + 1);
        }
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Up) {
        if (selectedIndex_ > 0) {
            setSelectedIndex(selectedIndex_ - 1);
        }
        ev.accept();
        return;
    }
}

void Dropdown::onFocusLost() {
    if (isOpen_) {
        close();
    }
    markPaintDirty();
}

void Dropdown::updateAnimations(float deltaSeconds) {
    popupAnim_.update(deltaSeconds);
    markPaintDirty();
}

bool Dropdown::hasActiveAnimations() const noexcept {
    return popupAnim_.isRunning();
}

void Dropdown::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    core::Color bg = isHovered_ ? hoverBgColor_ : bgColor_;
    core::Color border = isHovered_ ? hoverBorderColor_ : idleBorderColor_;

    ctx.drawBox(rect, bg, border, borderWidth_, cornerRadii_);

    if (hasKeyboardFocus()) {
        theme::paintFocusRing(ctx, rect, cornerRadii_[0]);
    }

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float padL = 12.0f;
    float chevronW = 24.0f;

    // Draw Selected text or Placeholder
    if (font_) {
        std::string display = selectedText();
        bool isPlaceholder = display.empty();
        if (isPlaceholder) {
            display = placeholder_;
        }

        core::Color textCol = isPlaceholder ? placeholderColor_ : textColor_;
        float textY = theme::computeCenteredTextY(targetCenterY, *font_);

        core::Rect clipText{
            core::Point{rect.origin.x + padL, rect.origin.y},
            core::Size{std::max(0.0f, rect.size.width - padL - chevronW), rect.size.height}};
        ctx.pushClip(clipText);
        auto layout = text::TextLayout::create(display, font_);
        ctx.drawTextLayout(core::Point{rect.origin.x + padL, textY}, layout, textCol);
        ctx.popClip();
    }

    // Draw Continuous Chevron Indicator
    float centerX = rect.origin.x + rect.size.width - 16.0f;
    float t = popupAnim_.currentValue();
    float tipY = targetCenterY + (2.5f - 4.5f * t);
    float armY = targetCenterY + (-2.0f + 4.5f * t);

    ctx.drawLine(core::Point{centerX - 4.5f, armY}, core::Point{centerX, tipY}, chevronColor_,
                 1.5f);
    ctx.drawLine(core::Point{centerX, tipY}, core::Point{centerX + 4.5f, armY}, chevronColor_,
                 1.5f);
}

} // namespace northern::widgets
