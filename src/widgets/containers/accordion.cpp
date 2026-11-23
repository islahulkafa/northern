#include "widgets/containers/accordion.hpp"

#include "layout/flex_solver.hpp"
#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace northern::widgets {

// CollapsibleSection
CollapsibleSection::CollapsibleSection(std::string title, bool expanded,
                                       std::shared_ptr<text::Font> font,
                                       SectionHeaderSize headerSize)
    : title_(std::move(title)), font_(std::move(font)), headerSize_(headerSize),
      isExpanded_(expanded), expandAnim_(expanded ? 1.0f : 0.0f) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::Arrow);
    style().flexDirection = layout::FlexDirection::Column;
    style().width = layout::Length::percent(100.0f);

    body_ = addChild(std::make_unique<Container>());
    body_->style().flexDirection = layout::FlexDirection::Column;
    body_->style().width = layout::Length::percent(100.0f);
    body_->style().padding = contentPadding_;
    if (isExpanded_) {
        body_->setVisible(true);
        body_->style().height = layout::Length::autoLength();
    } else {
        body_->setVisible(false);
        body_->style().height = layout::Length::points(0.0f);
    }
    applySizeMetrics();
}

float CollapsibleSection::headerHeight() const noexcept {
    return (headerSize_ == SectionHeaderSize::Compact) ? kHeaderHeightCompact
                                                       : kHeaderHeightStandard;
}

void CollapsibleSection::setHeaderSize(SectionHeaderSize size) noexcept {
    if (headerSize_ != size) {
        headerSize_ = size;
        applySizeMetrics();
    }
}

void CollapsibleSection::applySizeMetrics() noexcept {
    float h = headerHeight();
    style().padding.top = layout::Length::points(h);
    isTitleLayoutDirty_ = true;
    markLayoutDirty();
}

void CollapsibleSection::setIndicatorPlacement(IndicatorPlacement placement) noexcept {
    if (indicatorPlacement_ != placement) {
        indicatorPlacement_ = placement;
        markPaintDirty();
    }
}

void CollapsibleSection::setTitle(std::string title) {
    if (title_ != title) {
        title_ = std::move(title);
        isTitleLayoutDirty_ = true;
        markPaintDirty();
    }
}

void CollapsibleSection::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        isTitleLayoutDirty_ = true;
        markPaintDirty();
    }
}

void CollapsibleSection::setContentInternal(std::unique_ptr<Widget> content) {
    body_->clearChildren();
    if (content) {
        content_ = body_->addChild(std::move(content));
        content_->style().width = layout::Length::percent(100.0f);
    } else {
        content_ = nullptr;
    }
    markLayoutDirty();
}

void CollapsibleSection::setHeaderAccessoryInternal(std::unique_ptr<Widget> accessory) {
    if (headerAccessory_) {
        removeChild(headerAccessory_);
        headerAccessory_ = nullptr;
    }
    if (accessory) {
        accessory->style().positionType = layout::PositionType::Absolute;
        headerAccessory_ = addChild(std::move(accessory));
    }
    markLayoutDirty();
}

void CollapsibleSection::setContentPadding(const layout::RectLength& padding) noexcept {
    contentPadding_ = padding;
    if (body_) {
        body_->style().padding = contentPadding_;
        markLayoutDirty();
    }
}

float CollapsibleSection::calculateTargetHeight() {
    if (!content_) {
        return 0.0f;
    }
    float padV =
        contentPadding_.top.resolve(0.0f, 0.0f) + contentPadding_.bottom.resolve(0.0f, 0.0f);
    if (content_->style().height.isPoints()) {
        return content_->style().height.value + padV;
    }
    float w = layoutRect().size.width;
    if (w <= 0.0f && parent_) {
        w = parent_->layoutRect().size.width;
    }
    if (w <= 0.0f) {
        w = 300.0f;
    }
    float padH = contentPadding_.left.resolve(w, 0.0f) + contentPadding_.right.resolve(w, 0.0f);
    float availInnerW = std::max(0.0f, w - padH);
    layout::FlexSolver::solve(content_->layoutNode(), availInnerW, 0.0f);
    float h = content_->layoutNode().layoutRect().size.height;
    return h + padV;
}

void CollapsibleSection::setExpanded(bool expanded, bool animated) {
    if (isExpanded_ == expanded) {
        return;
    }
    isExpanded_ = expanded;
    if (animated) {
        if (isExpanded_) {
            body_->setVisible(true);
            targetBodyHeight_ = calculateTargetHeight();
            expandAnim_.start(expandAnim_.currentValue(), 1.0f, 0.25f,
                              core::animation::EasingCurve::InOutCubic);
        } else {
            targetBodyHeight_ = body_->layoutRect().size.height;
            if (targetBodyHeight_ <= 0.0f) {
                targetBodyHeight_ = calculateTargetHeight();
            }
            expandAnim_.start(expandAnim_.currentValue(), 0.0f, 0.25f,
                              core::animation::EasingCurve::InOutCubic);
        }
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
    } else {
        expandAnim_ = core::animation::PropertyAnimation<float>(isExpanded_ ? 1.0f : 0.0f);
        if (isExpanded_) {
            body_->setVisible(true);
            body_->style().height = layout::Length::autoLength();
        } else {
            body_->setVisible(false);
            body_->style().height = layout::Length::points(0.0f);
        }
    }
    markLayoutDirty();
    markPaintDirty();
    onToggled.emit(isExpanded_);
}

void CollapsibleSection::updateAnimations(float deltaSeconds) {
    expandAnim_.update(deltaSeconds);
    float t = expandAnim_.currentValue();
    if (expandAnim_.isRunning()) {
        body_->style().height = layout::Length::points(targetBodyHeight_ * t);
        markLayoutDirty();
        markPaintDirty();
    } else {
        if (isExpanded_) {
            body_->style().height = layout::Length::autoLength();
        } else {
            body_->setVisible(false);
            body_->style().height = layout::Length::points(0.0f);
        }
        markLayoutDirty();
        markPaintDirty();
    }
}

bool CollapsibleSection::hasActiveAnimations() const noexcept {
    return expandAnim_.isRunning();
}

void CollapsibleSection::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    float h = headerHeight();
    core::Rect headerRect{sRect.origin, {sRect.size.width, h}};

    const auto& pal = colorPalette();

    if (isPressedHeader_) {
        ctx.drawRect(headerRect, pal.surfaceRaised);
    } else if (isHoveredHeader_) {
        ctx.drawRect(headerRect, pal.surfaceRaised);
    }

    // Hairline divider below section header
    float dividerY = headerRect.origin.y + h;
    ctx.drawLine(core::Point{headerRect.origin.x, dividerY},
                 core::Point{headerRect.origin.x + headerRect.size.width, dividerY},
                 pal.borderSubtle, theme::Metrics::BorderWidthHairline);

    float t = std::clamp(expandAnim_.currentValue(), 0.0f, 1.0f);
    float angle = t * (std::numbers::pi_v<float> * 0.5f);
    core::Color chevronColor =
        (isHoveredHeader_ || isPressedHeader_ || isExpanded_) ? pal.textPrimary : pal.textSecondary;

    float chevSize = (headerSize_ == SectionHeaderSize::Compact) ? 12.0f : 14.0f;
    float chevCenterY = headerRect.origin.y + h * 0.5f;
    float chevCenterX = 0.0f;
    if (indicatorPlacement_ == IndicatorPlacement::Leading) {
        chevCenterX = headerRect.origin.x + 10.0f + chevSize * 0.5f;
    } else {
        chevCenterX = headerRect.origin.x + headerRect.size.width - 12.0f - chevSize * 0.5f;
    }

    float scale = chevSize / 14.0f;
    float vx0 = 2.0f * scale;
    float vy0 = 0.0f;
    float w1x0 = -2.0f * scale;
    float w1y0 = -3.8f * scale;
    float w2x0 = -2.0f * scale;
    float w2y0 = 3.8f * scale;

    float cosA = std::cos(angle);
    float sinA = std::sin(angle);

    core::Point pVertex{chevCenterX + (vx0 * cosA - vy0 * sinA),
                        chevCenterY + (vx0 * sinA + vy0 * cosA)};
    core::Point pWing1{chevCenterX + (w1x0 * cosA - w1y0 * sinA),
                       chevCenterY + (w1x0 * sinA + w1y0 * cosA)};
    core::Point pWing2{chevCenterX + (w2x0 * cosA - w2y0 * sinA),
                       chevCenterY + (w2x0 * sinA + w2y0 * cosA)};

    float strokeW = 1.5f;
    ctx.drawLine(pWing1, pVertex, chevronColor, strokeW);
    ctx.drawLine(pVertex, pWing2, chevronColor, strokeW);

    if (font_ && !title_.empty()) {
        if (isTitleLayoutDirty_ || !cachedTitleLayout_) {
            cachedTitleLayout_ = text::TextLayout::create(title_, font_);
            isTitleLayoutDirty_ = false;
        }
        float titleX = 0.0f;
        if (indicatorPlacement_ == IndicatorPlacement::Leading) {
            titleX = headerRect.origin.x + 10.0f + chevSize + 8.0f;
        } else {
            titleX = headerRect.origin.x + 10.0f;
        }
        float titleY = theme::computeCenteredTextY(chevCenterY, *font_);
        core::Color titleCol =
            (isHoveredHeader_ || isExpanded_) ? pal.textPrimary : pal.textSecondary;
        ctx.drawTextLayout(core::Point{titleX, titleY}, *cachedTitleLayout_, titleCol);
    }

    if (hasKeyboardFocus()) {
        theme::paintFocusRing(ctx, headerRect, 4.0f);
    }

    if (headerAccessory_ && headerAccessory_->isVisible()) {
        float accH = headerAccessory_->layoutNode().layoutRect().size.height;
        if (accH <= 0.0f) {
            accH = h - 8.0f;
        }
        float rightMargin =
            (indicatorPlacement_ == IndicatorPlacement::Trailing) ? (chevSize + 22.0f) : 10.0f;
        headerAccessory_->style().position.right = layout::Length::points(rightMargin);
        headerAccessory_->style().position.top = layout::Length::points((h - accH) * 0.5f);
    }
}

void CollapsibleSection::onMouseEnter() {}

void CollapsibleSection::onMouseLeave() {
    if (isHoveredHeader_ || isPressedHeader_) {
        isHoveredHeader_ = false;
        isPressedHeader_ = false;
        setCursorShape(platform::CursorShape::Arrow);
        markPaintDirty();
    }
}

void CollapsibleSection::onMouseMove(WidgetMouseEvent& ev) {
    if (ev.position().y <= headerHeight()) {
        if (!isHoveredHeader_) {
            isHoveredHeader_ = true;
            setCursorShape(platform::CursorShape::PointingHand);
            markPaintDirty();
        }
    } else {
        if (isHoveredHeader_) {
            isHoveredHeader_ = false;
            setCursorShape(platform::CursorShape::Arrow);
            markPaintDirty();
        }
    }
}

void CollapsibleSection::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && ev.position().y <= headerHeight()) {
        isPressedHeader_ = true;
        setFocus(FocusReason::Pointer);
        markPaintDirty();
        ev.accept();
    }
}

void CollapsibleSection::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && isPressedHeader_) {
        isPressedHeader_ = false;
        if (ev.position().y <= headerHeight()) {
            toggle();
        }
        markPaintDirty();
        ev.accept();
    }
}

void CollapsibleSection::onKeyDown(WidgetKeyEvent& ev) {
    if (ev.key() == platform::Key::Space || ev.key() == platform::Key::Enter) {
        toggle();
        ev.accept();
        return;
    }
    if (parent_) {
        if (auto* acc = dynamic_cast<Accordion*>(parent_)) {
            if (ev.key() == platform::Key::Down) {
                acc->focusNextSection(this);
                ev.accept();
                return;
            } else if (ev.key() == platform::Key::Up) {
                acc->focusPrevSection(this);
                ev.accept();
                return;
            } else if (ev.key() == platform::Key::Home) {
                acc->focusFirstSection();
                ev.accept();
                return;
            } else if (ev.key() == platform::Key::End) {
                acc->focusLastSection();
                ev.accept();
                return;
            }
        }
    }
}

// Accordion
Accordion::Accordion(bool singleExpanded, SectionHeaderSize headerSize)
    : singleExpanded_(singleExpanded), headerSize_(headerSize) {
    setFocusable(false);
    style().flexDirection = layout::FlexDirection::Column;
    style().width = layout::Length::percent(100.0f);
    style().rowGap = 0.0f;
}

CollapsibleSection* Accordion::addSection(std::unique_ptr<CollapsibleSection> section) {
    if (!section) {
        return nullptr;
    }
    CollapsibleSection* raw = addChild(std::move(section));
    raw->setHeaderSize(headerSize_);
    sections_.push_back(raw);

    raw->onToggled.connect([this, raw](bool expanded) {
        if (expanded && singleExpanded_) {
            for (auto* s : sections_) {
                if (s && s != raw && s->isExpanded()) {
                    s->setExpanded(false, true);
                }
            }
        }
        auto idxOpt = indexOfSection(raw);
        if (idxOpt.has_value()) {
            onSectionToggled.emit(*idxOpt, expanded);
        }
    });

    if (singleExpanded_ && raw->isExpanded()) {
        for (auto* s : sections_) {
            if (s && s != raw && s->isExpanded()) {
                s->setExpanded(false, false);
            }
        }
    }

    return raw;
}

CollapsibleSection* Accordion::addSection(std::string title, std::unique_ptr<Widget> content,
                                          bool expanded) {
    auto sec =
        std::make_unique<CollapsibleSection>(std::move(title), expanded, nullptr, headerSize_);
    if (content) {
        sec->setContent(std::move(content));
    }
    return addSection(std::move(sec));
}

void Accordion::removeSection(size_t index) {
    if (index < sections_.size()) {
        CollapsibleSection* s = sections_[index];
        sections_.erase(sections_.begin() + index);
        removeChild(s);
    }
}

void Accordion::clearSections() {
    sections_.clear();
    clearChildren();
}

CollapsibleSection* Accordion::sectionAt(size_t index) const {
    if (index < sections_.size()) {
        return sections_[index];
    }
    return nullptr;
}

std::optional<size_t> Accordion::indexOfSection(const CollapsibleSection* section) const noexcept {
    for (size_t i = 0; i < sections_.size(); ++i) {
        if (sections_[i] == section) {
            return i;
        }
    }
    return std::nullopt;
}

void Accordion::setSingleExpanded(bool singleExpanded) {
    singleExpanded_ = singleExpanded;
    if (singleExpanded_) {
        bool foundFirst = false;
        for (auto* s : sections_) {
            if (s && s->isExpanded()) {
                if (!foundFirst) {
                    foundFirst = true;
                } else {
                    s->setExpanded(false, true);
                }
            }
        }
    }
}

void Accordion::setHeaderSize(SectionHeaderSize size) noexcept {
    headerSize_ = size;
    for (auto* s : sections_) {
        if (s) {
            s->setHeaderSize(size);
        }
    }
}

void Accordion::focusNextSection(CollapsibleSection* current) {
    if (sections_.empty()) {
        return;
    }
    auto idxOpt = indexOfSection(current);
    size_t nextIdx = 0;
    if (idxOpt.has_value() && *idxOpt + 1 < sections_.size()) {
        nextIdx = *idxOpt + 1;
    }
    if (nextIdx < sections_.size() && sections_[nextIdx]) {
        sections_[nextIdx]->setFocus(FocusReason::Keyboard);
    }
}

void Accordion::focusPrevSection(CollapsibleSection* current) {
    if (sections_.empty()) {
        return;
    }
    auto idxOpt = indexOfSection(current);
    size_t prevIdx = sections_.size() - 1;
    if (idxOpt.has_value() && *idxOpt > 0) {
        prevIdx = *idxOpt - 1;
    }
    if (prevIdx < sections_.size() && sections_[prevIdx]) {
        sections_[prevIdx]->setFocus(FocusReason::Keyboard);
    }
}

void Accordion::focusFirstSection() {
    if (!sections_.empty() && sections_.front()) {
        sections_.front()->setFocus(FocusReason::Keyboard);
    }
}

void Accordion::focusLastSection() {
    if (!sections_.empty() && sections_.back()) {
        sections_.back()->setFocus(FocusReason::Keyboard);
    }
}

} // namespace northern::widgets
