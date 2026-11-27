#include "widgets/containers/accordion.hpp"

#include "layout/flex_solver.hpp"
#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/label.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace northern::widgets {

namespace {
constexpr float kExpandAnimDuration = 0.25f;
constexpr float kHoverAnimDuration = 0.12f;
constexpr float kHeaderPillInset = 4.0f;
constexpr float kHorizontalInset = 16.0f;
constexpr float kBodyLeftIndent = 20.0f;
} // namespace

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
    body_->setClipsChildren(true);
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
    float base =
        (headerSize_ == SectionHeaderSize::Compact) ? kHeaderHeightCompact : kHeaderHeightStandard;
    return base * dpiScale_;
}

void CollapsibleSection::setHeaderSize(SectionHeaderSize size) noexcept {
    if (headerSize_ != size) {
        headerSize_ = size;
        resolvedDefaultFont_.reset();
        applySizeMetrics();
    }
}

void CollapsibleSection::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiMetrics(scale);
}

void CollapsibleSection::applyDpiMetrics(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        if (!hasCustomPadding_) {
            contentPadding_ = layout::RectLength{
                .left = layout::Length::points(kBodyLeftIndent * dpiScale_),
                .top = layout::Length::points(2.0f * dpiScale_),
                .right = layout::Length::points(kHorizontalInset * dpiScale_),
                .bottom = layout::Length::points(2.0f * dpiScale_),
            };
            if (body_) {
                body_->style().padding = contentPadding_;
            }
        }
        applySizeMetrics();
    }
}

std::shared_ptr<text::Font> CollapsibleSection::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float basePx = (headerSize_ == SectionHeaderSize::Compact) ? 13.5f : 14.5f;
        resolvedDefaultFont_ =
            text::FontManager::instance().semiBoldFont(basePx * dpiScale_).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void CollapsibleSection::applySizeMetrics() noexcept {
    float h = headerHeight();
    style().padding.top = layout::Length::points(h);
    updateAccessoryLayout();
    isTitleLayoutDirty_ = true;
    markLayoutDirty();
    markPaintDirty();
}

void CollapsibleSection::updateAccessoryLayout() noexcept {
    if (!headerAccessory_) {
        return;
    }
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float h = headerHeight();
    float chevSize = ((headerSize_ == SectionHeaderSize::Compact) ? 12.0f : 14.0f) * scale;
    float rightMargin = (indicatorPlacement_ == IndicatorPlacement::Trailing)
                            ? (chevSize + (kHorizontalInset + 10.0f) * scale)
                            : (kHorizontalInset * scale);
    headerAccessory_->style().position.right = layout::Length::points(rightMargin);

    if (auto* accLbl = dynamic_cast<Label*>(headerAccessory_)) {
        accLbl->setVerticalAlignment(VerticalAlignment::Center);
        accLbl->style().height = layout::Length::points(h);
        accLbl->style().position.top = layout::Length::points(-h);
        return;
    }

    float accH = headerAccessory_->layoutNode().layoutRect().size.height;
    if (accH <= 0.0f) {
        layout::FlexSolver::solve(headerAccessory_->layoutNode(), 200.0f * scale, h);
        accH = headerAccessory_->layoutNode().layoutRect().size.height;
    }
    if (accH <= 0.0f) {
        accH = h - 8.0f * scale;
    }
    headerAccessory_->style().position.top = layout::Length::points(-(h + accH) * 0.5f);
}

void CollapsibleSection::setIndicatorPlacement(IndicatorPlacement placement) noexcept {
    if (indicatorPlacement_ != placement) {
        indicatorPlacement_ = placement;
        updateAccessoryLayout();
        markLayoutDirty();
        markPaintDirty();
    }
}

void CollapsibleSection::setCardStyled(bool cardStyled) noexcept {
    if (cardStyled_ != cardStyled) {
        cardStyled_ = cardStyled;
        markPaintDirty();
    }
}

void CollapsibleSection::setShowRowDividers(bool show) noexcept {
    if (showRowDividers_ != show) {
        showRowDividers_ = show;
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
        resolvedDefaultFont_.reset();
        isTitleLayoutDirty_ = true;
        markPaintDirty();
    }
}

void CollapsibleSection::setContentInternal(std::unique_ptr<Widget> content) {
    body_->clearChildren();
    if (content) {
        content_ = body_->addChild(std::move(content));
        content_->style().width = layout::Length::percent(100.0f);
        content_->style().flexShrink = 0.0f;
    } else {
        content_ = nullptr;
    }
    markLayoutDirty();
}

void CollapsibleSection::addContentInternal(std::unique_ptr<Widget> item) {
    if (!item || !body_) {
        return;
    }
    Widget* raw = body_->addChild(std::move(item));
    raw->style().width = layout::Length::percent(100.0f);
    raw->style().flexShrink = 0.0f;
    if (!content_) {
        content_ = raw;
    }
    markLayoutDirty();
}

Container* CollapsibleSection::addRow(std::string label, std::unique_ptr<Widget> trailing) {
    auto row = std::make_unique<Container>();
    row->setFlexDirection(layout::FlexDirection::Row);
    row->setJustifyContent(layout::JustifyContent::SpaceBetween);
    row->setAlignItems(layout::AlignItems::Center);
    row->style().width = layout::Length::percent(100.0f);
    row->style().height = layout::Length::points(36.0f * dpiScale_);

    auto rowFont =
        font_ ? font_
              : text::FontManager::instance().defaultFont(14.0f * dpiScale_).valueOr(nullptr);
    auto lbl = std::make_unique<Label>(std::move(label), rowFont);
    lbl->setDpiScale(dpiScale_);
    lbl->setVerticalAlignment(VerticalAlignment::Center);
    lbl->style().height = layout::Length::percent(100.0f);
    lbl->style().flexGrow = 1.0f;
    lbl->style().flexShrink = 1.0f;
    row->addChild(std::move(lbl));

    if (trailing) {
        if (auto* trailingLbl = dynamic_cast<Label*>(trailing.get())) {
            trailingLbl->setVerticalAlignment(VerticalAlignment::Center);
            trailingLbl->style().height = layout::Length::percent(100.0f);
            if (!trailingLbl->font()) {
                trailingLbl->setFont(rowFont);
            }
        }
        row->addChild(std::move(trailing));
    }

    return addContent(std::move(row));
}

void CollapsibleSection::setHeaderAccessoryInternal(std::unique_ptr<Widget> accessory) {
    if (headerAccessory_) {
        removeChild(headerAccessory_);
        headerAccessory_ = nullptr;
    }
    if (accessory) {
        accessory->style().positionType = layout::PositionType::Absolute;
        headerAccessory_ = addChild(std::move(accessory));
        updateAccessoryLayout();
    }
    markLayoutDirty();
}

void CollapsibleSection::setContentPadding(const layout::RectLength& padding) noexcept {
    hasCustomPadding_ = true;
    contentPadding_ = padding;
    if (body_) {
        body_->style().padding = contentPadding_;
        markLayoutDirty();
    }
}

float CollapsibleSection::calculateTargetHeight() {
    if (!body_ || body_->childCount() == 0) {
        return 0.0f;
    }
    float w = layoutRect().size.width;
    if (w <= 0.0f && parent_) {
        w = parent_->layoutRect().size.width;
    }
    if (w <= 0.0f) {
        w = 320.0f * dpiScale_;
    }

    layout::Length prevHeight = body_->style().height;
    body_->style().height = layout::Length::autoLength();
    body_->layoutNode().markDirty();
    layout::FlexSolver::solve(body_->layoutNode(), w, 0.0f);
    float h = body_->layoutNode().layoutRect().size.height;
    body_->style().height = prevHeight;
    body_->layoutNode().markDirty();
    return h;
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
            expandAnim_.start(expandAnim_.currentValue(), 1.0f, kExpandAnimDuration,
                              core::animation::EasingCurve::InOutCubic);
        } else {
            targetBodyHeight_ = body_->layoutRect().size.height;
            if (targetBodyHeight_ <= 0.0f) {
                targetBodyHeight_ = calculateTargetHeight();
            }
            expandAnim_.start(expandAnim_.currentValue(), 0.0f, kExpandAnimDuration,
                              core::animation::EasingCurve::InOutCubic);
        }
        body_->style().height =
            layout::Length::points(targetBodyHeight_ * expandAnim_.currentValue());
        wasExpandAnimRunning_ = true;
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
    } else {
        wasExpandAnimRunning_ = false;
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

void CollapsibleSection::updateHoverAnimation(bool hovered) {
    float target = hovered ? 1.0f : 0.0f;
    hoverAnim_.start(hoverAnim_.currentValue(), target, kHoverAnimDuration,
                     core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
}

void CollapsibleSection::updateAnimations(float deltaSeconds) {
    bool expandRunning = expandAnim_.isRunning();
    expandAnim_.update(deltaSeconds);
    hoverAnim_.update(deltaSeconds);

    if (expandRunning || wasExpandAnimRunning_) {
        float t = expandAnim_.currentValue();
        if (expandAnim_.isRunning()) {
            body_->style().height = layout::Length::points(targetBodyHeight_ * t);
        } else {
            wasExpandAnimRunning_ = false;
            if (isExpanded_) {
                body_->style().height = layout::Length::autoLength();
            } else {
                body_->setVisible(false);
                body_->style().height = layout::Length::points(0.0f);
            }
        }
        markLayoutDirty();
    }
    markPaintDirty();
}

bool CollapsibleSection::hasActiveAnimations() const noexcept {
    return expandAnim_.isRunning() || hoverAnim_.isRunning();
}

void CollapsibleSection::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float h = headerHeight();
    core::Rect headerRect{sRect.origin, {sRect.size.width, h}};
    const auto& pal = colorPalette();
    const bool enabled = isEnabled();

    if (cardStyled_) {
        float radius = theme::Metrics::CornerRadiusOverlay * scale;
        ctx.drawRoundedRect(sRect, pal.surfaceDefault, radius);
        ctx.drawBorder(sRect, pal.borderSubtle, theme::Metrics::BorderWidthHairline * scale,
                       radius);
    }

    float pillInset = kHeaderPillInset * scale;
    core::Rect headerPillRect{
        core::Point{headerRect.origin.x + pillInset, headerRect.origin.y + pillInset},
        core::Size{std::max(0.0f, headerRect.size.width - pillInset * 2.0f),
                   std::max(0.0f, headerRect.size.height - pillInset * 2.0f)}};
    float concentricPillRadius =
        theme::resolveConcentricRadius(theme::Metrics::CornerRadiusOverlay, kHeaderPillInset) *
        scale;

    float ht = std::clamp(hoverAnim_.currentValue(), 0.0f, 1.0f);
    if (enabled && (isPressedHeader_ || ht > 0.001f)) {
        float alpha = isPressedHeader_ ? 0.85f : (0.55f * ht);
        ctx.drawRoundedRect(headerPillRect, pal.surfaceRaised.withAlpha(alpha),
                            concentricPillRadius);
    }

    float t = std::clamp(expandAnim_.currentValue(), 0.0f, 1.0f);
    if (t > 0.01f && body_ && body_->childCount() > 0) {
        float insetX = kHorizontalInset * scale;
        float dividerY = headerRect.origin.y + h;
        if (sRect.size.width > insetX * 2.0f) {
            ctx.drawLine(core::Point{sRect.origin.x + insetX, dividerY},
                         core::Point{sRect.origin.x + sRect.size.width - insetX, dividerY},
                         pal.borderSubtle.withAlpha(pal.borderSubtle.a * t),
                         theme::Metrics::BorderWidthHairline * scale);
        }
    }

    if (showRowDividers_ && t > 0.01f && body_ && body_->isVisible() && body_->childCount() > 1) {
        float leftX = sRect.origin.x + kBodyLeftIndent * scale;
        float rightX = sRect.origin.x + sRect.size.width - kHorizontalInset * scale;
        if (rightX > leftX) {
            for (size_t i = 0; i + 1 < body_->childCount(); ++i) {
                Widget* child = body_->childAt(i);
                if (!child || !child->isVisible()) {
                    continue;
                }
                core::Rect childRect = child->screenRect();
                float rowDivY = childRect.origin.y + childRect.size.height;
                ctx.drawLine(core::Point{leftX, rowDivY}, core::Point{rightX, rowDivY},
                             pal.borderSubtle.withAlpha(pal.borderSubtle.a * t),
                             theme::Metrics::BorderWidthHairline * scale);
            }
        }
    }

    float angle = t * (std::numbers::pi_v<float> * 0.5f);
    bool isAccent = enabled && (isHoveredHeader_ || isPressedHeader_ || isExpanded_);
    core::Color chevronColor =
        !enabled ? pal.textDisabled : (isAccent ? pal.accentDefault : pal.textSecondary);

    float chevSize = ((headerSize_ == SectionHeaderSize::Compact) ? 12.0f : 14.0f) * scale;
    float chevCenterY = headerRect.origin.y + h * 0.5f;
    float chevCenterX = 0.0f;
    if (indicatorPlacement_ == IndicatorPlacement::Leading) {
        chevCenterX = headerRect.origin.x + kHorizontalInset * scale + chevSize * 0.5f;
    } else {
        chevCenterX = headerRect.origin.x + headerRect.size.width - kHorizontalInset * scale -
                      chevSize * 0.5f;
    }

    float chevScale = chevSize / 14.0f;
    float vx0 = 2.0f * chevScale;
    float vy0 = 0.0f;
    float w1x0 = -2.0f * chevScale;
    float w1y0 = -3.8f * chevScale;
    float w2x0 = -2.0f * chevScale;
    float w2y0 = 3.8f * chevScale;

    float cosA = std::cos(angle);
    float sinA = std::sin(angle);

    core::Point pVertex{chevCenterX + (vx0 * cosA - vy0 * sinA),
                        chevCenterY + (vx0 * sinA + vy0 * cosA)};
    core::Point pWing1{chevCenterX + (w1x0 * cosA - w1y0 * sinA),
                       chevCenterY + (w1x0 * sinA + w1y0 * cosA)};
    core::Point pWing2{chevCenterX + (w2x0 * cosA - w2y0 * sinA),
                       chevCenterY + (w2x0 * sinA + w2y0 * cosA)};

    ctx.drawLine(pWing1, pVertex, chevronColor, 1.75f * scale);
    ctx.drawLine(pVertex, pWing2, chevronColor, 1.75f * scale);

    auto f = effectiveFont();
    if (f && !title_.empty()) {
        if (isTitleLayoutDirty_ || !cachedTitleLayout_) {
            cachedTitleLayout_ = text::TextLayout::create(title_, f);
            isTitleLayoutDirty_ = false;
        }
        float titleX = 0.0f;
        if (indicatorPlacement_ == IndicatorPlacement::Leading) {
            titleX = headerRect.origin.x + kHorizontalInset * scale + chevSize + 8.0f * scale;
        } else {
            titleX = headerRect.origin.x + kHorizontalInset * scale;
        }
        float titleY = theme::computeCenteredTextY(chevCenterY, *f);
        core::Color titleCol =
            !enabled ? pal.textDisabled
                     : ((isHoveredHeader_ || isPressedHeader_ || isExpanded_) ? pal.textPrimary
                                                                              : pal.textSecondary);
        ctx.drawTextLayout(core::Point{titleX, titleY}, *cachedTitleLayout_, titleCol);
    }

    if (enabled && hasKeyboardFocus()) {
        paintFocusRing(ctx, headerPillRect, concentricPillRadius);
    }
}

void CollapsibleSection::onContextAttached() {
    if (!hasCustomDpiScale_) {
        float ctxDpi = contextDpiScale();
        if (ctxDpi > 0.0f && ctxDpi != dpiScale_) {
            applyDpiMetrics(ctxDpi);
        }
    }
    isTitleLayoutDirty_ = true;
    markPaintDirty();
}

void CollapsibleSection::onThemeChanged() {
    isTitleLayoutDirty_ = true;
    markPaintDirty();
}

void CollapsibleSection::onFocusLost() {
    if (isPressedHeader_) {
        isPressedHeader_ = false;
        if (context_ && context_->mouseGrabWidget() == this) {
            context_->releaseMouseGrab();
        }
        markPaintDirty();
    }
    Widget::onFocusLost();
}

void CollapsibleSection::onMouseEnter() {}

void CollapsibleSection::onMouseLeave() {
    if (context_ && context_->mouseGrabWidget() == this) {
        context_->releaseMouseGrab();
    }
    if (isHoveredHeader_ || isPressedHeader_) {
        isHoveredHeader_ = false;
        isPressedHeader_ = false;
        setCursorShape(platform::CursorShape::Arrow);
        updateHoverAnimation(false);
        markPaintDirty();
    }
}

void CollapsibleSection::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        onMouseLeave();
        return;
    }
    float w = layoutRect().size.width;
    bool inHeader = ev.position().y >= 0.0f && ev.position().y <= headerHeight() &&
                    (w <= 0.0f || (ev.position().x >= 0.0f && ev.position().x <= w));
    if (inHeader) {
        if (!isHoveredHeader_) {
            isHoveredHeader_ = true;
            setCursorShape(platform::CursorShape::PointingHand);
            updateHoverAnimation(true);
            markPaintDirty();
        }
    } else {
        if (isHoveredHeader_) {
            isHoveredHeader_ = false;
            setCursorShape(platform::CursorShape::Arrow);
            updateHoverAnimation(false);
            markPaintDirty();
        }
    }
}

void CollapsibleSection::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.isPrimaryButton() && ev.position().y >= 0.0f && ev.position().y <= headerHeight()) {
        isPressedHeader_ = true;
        setFocus(FocusReason::Pointer);
        if (context_) {
            context_->setMouseGrabWidget(this);
        }
        markPaintDirty();
        ev.accept();
    }
}

void CollapsibleSection::onMouseUp(WidgetMouseEvent& ev) {
    if (context_ && context_->mouseGrabWidget() == this) {
        context_->releaseMouseGrab();
    }
    if (!isEnabled() || !isPressedHeader_) {
        isPressedHeader_ = false;
        return;
    }
    if (ev.isPrimaryButton()) {
        isPressedHeader_ = false;
        float w = layoutRect().size.width;
        bool inHeader = ev.position().y >= 0.0f && ev.position().y <= headerHeight() &&
                        (w <= 0.0f || (ev.position().x >= 0.0f && ev.position().x <= w));
        if (inHeader) {
            toggle();
        }
        markPaintDirty();
        ev.accept();
    }
}

void CollapsibleSection::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled()) {
        return;
    }
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
Accordion::Accordion(bool singleExpanded, SectionHeaderSize headerSize, AccordionStyle style)
    : singleExpanded_(singleExpanded), headerSize_(headerSize), accordionStyle_(style) {
    setFocusable(false);
    this->style().flexDirection = layout::FlexDirection::Column;
    this->style().width = layout::Length::percent(100.0f);
    applyStyleToSections();
}

CollapsibleSection* Accordion::addSection(std::unique_ptr<CollapsibleSection> section) {
    if (!section) {
        return nullptr;
    }
    CollapsibleSection* raw = addChild(std::move(section));
    raw->setHeaderSize(headerSize_);
    raw->setDpiScale(dpiScale_);
    raw->setCardStyled(accordionStyle_ == AccordionStyle::Separated);
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

void Accordion::setAccordionStyle(AccordionStyle style) {
    if (accordionStyle_ != style) {
        accordionStyle_ = style;
        applyStyleToSections();
        markLayoutDirty();
        markPaintDirty();
    }
}

void Accordion::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiMetrics(scale);
}

void Accordion::applyDpiMetrics(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        for (auto* s : sections_) {
            if (s) {
                s->setDpiScale(clamped);
            }
        }
        applyStyleToSections();
        markLayoutDirty();
        markPaintDirty();
    }
}

void Accordion::onContextAttached() {
    if (!hasCustomDpiScale_) {
        float ctxDpi = contextDpiScale();
        if (ctxDpi > 0.0f && ctxDpi != dpiScale_) {
            applyDpiMetrics(ctxDpi);
        }
    }
    markPaintDirty();
}

void Accordion::onThemeChanged() {
    markPaintDirty();
}

void Accordion::applyStyleToSections() {
    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    bool separated = (accordionStyle_ == AccordionStyle::Separated);
    style().rowGap = separated ? (8.0f * scale) : 0.0f;
    for (auto* s : sections_) {
        if (s) {
            s->setCardStyled(separated);
        }
    }
}

void Accordion::paint(renderer::PaintContext& ctx) {
    if (sections_.empty() || accordionStyle_ == AccordionStyle::Separated) {
        return;
    }

    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    const auto& pal = colorPalette();

    if (accordionStyle_ == AccordionStyle::Grouped) {
        float radius = theme::Metrics::CornerRadiusOverlay * scale;
        ctx.drawRoundedRect(sRect, pal.surfaceDefault, radius);
        ctx.drawBorder(sRect, pal.borderSubtle, theme::Metrics::BorderWidthHairline * scale,
                       radius);
    }

    float insetX = (accordionStyle_ == AccordionStyle::Grouped) ? (kHorizontalInset * scale) : 0.0f;
    float leftX = sRect.origin.x + insetX;
    float rightX = sRect.origin.x + sRect.size.width - insetX;
    if (rightX <= leftX) {
        return;
    }

    for (size_t i = 0; i < sections_.size(); ++i) {
        CollapsibleSection* sec = sections_[i];
        if (!sec || !sec->isVisible()) {
            continue;
        }
        bool hasNextVisible = false;
        for (size_t j = i + 1; j < sections_.size(); ++j) {
            if (sections_[j] && sections_[j]->isVisible()) {
                hasNextVisible = true;
                break;
            }
        }
        if (!hasNextVisible) {
            break;
        }
        core::Rect secRect = sec->screenRect();
        float divY = secRect.origin.y + secRect.size.height;
        ctx.drawLine(core::Point{leftX, divY}, core::Point{rightX, divY}, pal.borderSubtle,
                     theme::Metrics::BorderWidthHairline * scale);
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
