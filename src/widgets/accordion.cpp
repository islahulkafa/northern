#include "widgets/accordion.hpp"

#include "layout/flex_solver.hpp"
#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/container.hpp"
#include "widgets/icons.hpp"
#include "widgets/theme.hpp"
#include "widgets/widget_context.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace northern::widgets {

// ============================================================================
// CollapsibleSection
// ============================================================================

CollapsibleSection::CollapsibleSection(std::string title, bool expanded,
                                       std::shared_ptr<text::Font> font)
    : title_(std::move(title)), font_(std::move(font)), isExpanded_(expanded),
      expandAnim_(expanded ? 1.0f : 0.0f) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::Arrow);
    style().flexDirection = layout::FlexDirection::Column;
    style().width = layout::Length::percent(100.0f);
    style().padding.top = layout::Length::points(kHeaderHeight);

    body_ = addChild(std::make_unique<Container>());
    body_->style().flexDirection = layout::FlexDirection::Column;
    body_->style().width = layout::Length::percent(100.0f);
    if (isExpanded_) {
        body_->setVisible(true);
        body_->style().height = layout::Length::autoLength();
    } else {
        body_->setVisible(false);
        body_->style().height = layout::Length::points(0.0f);
    }
}

void CollapsibleSection::setTitle(std::string title) {
    if (title_ != title) {
        title_ = std::move(title);
        markPaintDirty();
    }
}

void CollapsibleSection::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
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

float CollapsibleSection::calculateTargetHeight() {
    if (!content_) {
        return 0.0f;
    }
    if (content_->style().height.isPoints()) {
        return content_->style().height.value;
    }
    float w = layoutRect().size.width;
    if (w <= 0.0f && parent_) {
        w = parent_->layoutRect().size.width;
    }
    if (w <= 0.0f) {
        w = 300.0f;
    }
    layout::FlexSolver::solve(content_->layoutNode(), w, 0.0f);
    float h = content_->layoutNode().layoutRect().size.height;
    return std::max(20.0f, h);
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
            expandAnim_.start(expandAnim_.currentValue(), 1.0f, 0.15f,
                              core::animation::EasingCurve::OutCubic);
        } else {
            targetBodyHeight_ = body_->layoutRect().size.height;
            if (targetBodyHeight_ <= 0.0f) {
                targetBodyHeight_ = calculateTargetHeight();
            }
            expandAnim_.start(expandAnim_.currentValue(), 0.0f, 0.15f,
                              core::animation::EasingCurve::OutCubic);
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
    core::Rect headerRect{sRect.origin, {sRect.size.width, kHeaderHeight}};

    if (isHoveredHeader_) {
        ctx.drawRect(headerRect, theme::ColorTokens::SurfaceRaised);
    }

    float t = std::clamp(expandAnim_.currentValue(), 0.0f, 1.0f);
    core::Color chevronColor =
        isHoveredHeader_ ? theme::ColorTokens::TextPrimary : theme::ColorTokens::TextSecondary;
    IconId chevId = (t > 0.5f) ? IconId::ChevronDown : IconId::ChevronRight;
    if (auto doc = getSvgDocument(chevId)) {
        core::Rect chevRect{
            core::Point{headerRect.origin.x + 8.0f,
                        headerRect.origin.y + (kHeaderHeight - 14.0f) * 0.5f},
            core::Size{14.0f, 14.0f}};
        ctx.drawSvg(*doc, chevRect, chevronColor);
    }

    if (font_ && !title_.empty()) {
        float titleX = headerRect.origin.x + 28.0f;
        float titleY =
            theme::computeCenteredTextY(headerRect.origin.y + kHeaderHeight * 0.5f, *font_);
        core::Color titleCol = (isHoveredHeader_ || isExpanded_)
                                   ? theme::ColorTokens::TextPrimary
                                   : theme::ColorTokens::TextSecondary;
        auto layout = text::TextLayout::create(title_, font_);
        ctx.drawTextLayout(core::Point{titleX, titleY}, layout, titleCol);
    }

    if (hasKeyboardFocus()) {
        theme::paintFocusRing(ctx, headerRect, 4.0f);
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
    if (ev.position().y <= kHeaderHeight) {
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
    if (ev.button() == platform::MouseButton::Left && ev.position().y <= kHeaderHeight) {
        isPressedHeader_ = true;
        setFocus(FocusReason::Pointer);
        markPaintDirty();
        ev.accept();
    }
}

void CollapsibleSection::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && isPressedHeader_) {
        isPressedHeader_ = false;
        if (ev.position().y <= kHeaderHeight) {
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
    }
}

// ============================================================================
// Accordion
// ============================================================================

Accordion::Accordion(bool singleExpanded) : singleExpanded_(singleExpanded) {
    setFocusable(false);
    style().flexDirection = layout::FlexDirection::Column;
    style().width = layout::Length::percent(100.0f);
    style().rowGap = 2.0f;
}

CollapsibleSection* Accordion::addSection(std::unique_ptr<CollapsibleSection> section) {
    if (!section) {
        return nullptr;
    }
    CollapsibleSection* raw = addChild(std::move(section));
    size_t idx = sections_.size();
    sections_.push_back(raw);

    raw->onToggled.connect([this, raw, idx](bool expanded) {
        if (expanded && singleExpanded_) {
            for (auto* s : sections_) {
                if (s && s != raw && s->isExpanded()) {
                    s->setExpanded(false, true);
                }
            }
        }
        onSectionToggled.emit(idx, expanded);
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
    auto sec = std::make_unique<CollapsibleSection>(std::move(title), expanded);
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

} // namespace northern::widgets
