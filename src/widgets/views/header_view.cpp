#include "widgets/views/header_view.hpp"

#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/icons.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

HeaderView::HeaderView(std::shared_ptr<text::Font> font) : font_(std::move(font)) {
    setFocusable(false);
    setCursorShape(platform::CursorShape::Arrow);
    style().height = layout::Length::points(kDefaultHeaderHeight);
    style().width = layout::Length::percent(100.0f);
}

void HeaderView::addSection(std::string title, float width, float minWidth) {
    sections_.push_back(HeaderSection{
        .title = std::move(title),
        .width = std::max(minWidth, width),
        .minWidth = minWidth,
        .visible = true,
        .sortOrder = SortOrder::None,
        .alignment = text::TextAlignment::Left,
    });
    invalidateSectionCaches();
    markLayoutDirty();
}

void HeaderView::insertSection(size_t index, std::string title, float width, float minWidth) {
    size_t clamped = std::min(index, sections_.size());
    sections_.insert(sections_.begin() + clamped, HeaderSection{
                                                      .title = std::move(title),
                                                      .width = std::max(minWidth, width),
                                                      .minWidth = minWidth,
                                                      .visible = true,
                                                      .sortOrder = SortOrder::None,
                                                      .alignment = text::TextAlignment::Left,
                                                  });
    invalidateSectionCaches();
    markLayoutDirty();
}

void HeaderView::removeSection(size_t index) {
    if (index < sections_.size()) {
        sections_.erase(sections_.begin() + index);
        invalidateSectionCaches();
        markLayoutDirty();
    }
}

void HeaderView::clearSections() {
    sections_.clear();
    invalidateSectionCaches();
    markLayoutDirty();
}

void HeaderView::setSectionTitle(size_t index, std::string title) {
    if (index < sections_.size()) {
        sections_[index].title = std::move(title);
        if (index < sectionCaches_.size()) {
            sectionCaches_[index].valid = false;
            sectionCaches_[index].layout.reset();
        }
        markPaintDirty();
    }
}

std::string HeaderView::sectionTitle(size_t index) const {
    if (index < sections_.size()) {
        return sections_[index].title;
    }
    return {};
}

void HeaderView::setSectionWidth(size_t index, float width) {
    if (index < sections_.size()) {
        float clamped = std::max(sections_[index].minWidth, width);
        if (sections_[index].width != clamped) {
            sections_[index].width = clamped;
            markLayoutDirty();
            onSectionResized.emit(index, clamped);
        }
    }
}

float HeaderView::sectionWidth(size_t index) const {
    if (index < sections_.size()) {
        return resolvedSectionWidth(index);
    }
    return 0.0f;
}

float HeaderView::sectionPosition(size_t index) const {
    float pos = 0.0f;
    for (size_t i = 0; i < index && i < sections_.size(); ++i) {
        if (sections_[i].visible) {
            pos += resolvedSectionWidth(i);
        }
    }
    return pos;
}

void HeaderView::setSectionSortOrder(size_t index, SortOrder order) {
    if (index < sections_.size()) {
        for (size_t i = 0; i < sections_.size(); ++i) {
            if (i != index) {
                sections_[i].sortOrder = SortOrder::None;
            }
        }
        sections_[index].sortOrder = order;
        markPaintDirty();
        onSortChanged.emit(index, order);
    }
}

SortOrder HeaderView::sectionSortOrder(size_t index) const {
    if (index < sections_.size()) {
        return sections_[index].sortOrder;
    }
    return SortOrder::None;
}

int HeaderView::sortedSection() const noexcept {
    for (size_t i = 0; i < sections_.size(); ++i) {
        if (sections_[i].sortOrder != SortOrder::None) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void HeaderView::setStretchLastSection(bool stretch) noexcept {
    if (stretchLastSection_ != stretch) {
        stretchLastSection_ = stretch;
        markLayoutDirty();
    }
}

void HeaderView::setHorizontalOffset(float offset) noexcept {
    float clamped = std::max(0.0f, offset);
    if (horizontalOffset_ != clamped) {
        horizontalOffset_ = clamped;
        markPaintDirty();
    }
}

float HeaderView::totalWidth() const noexcept {
    float total = 0.0f;
    for (size_t i = 0; i < sections_.size(); ++i) {
        if (sections_[i].visible) {
            total += resolvedSectionWidth(i);
        }
    }
    return total;
}

float HeaderView::resolvedSectionWidth(size_t index) const {
    if (index >= sections_.size() || !sections_[index].visible) {
        return 0.0f;
    }

    if (stretchLastSection_) {
        bool isLastVisible = true;
        for (size_t i = index + 1; i < sections_.size(); ++i) {
            if (sections_[i].visible) {
                isLastVisible = false;
                break;
            }
        }

        if (isLastVisible) {
            float prevW = 0.0f;
            for (size_t i = 0; i < index; ++i) {
                if (sections_[i].visible) {
                    prevW += sections_[i].width;
                }
            }
            float containerW = layoutRect().size.width;
            if (containerW > prevW + sections_[index].minWidth) {
                return std::max(sections_[index].minWidth, containerW - prevW);
            }
        }
    }

    return sections_[index].width;
}

int HeaderView::sectionAt(float x) const noexcept {
    float cur = 0.0f;
    for (size_t i = 0; i < sections_.size(); ++i) {
        if (!sections_[i].visible) {
            continue;
        }
        float w = resolvedSectionWidth(i);
        if (x >= cur && x < cur + w) {
            return static_cast<int>(i);
        }
        cur += w;
    }
    return -1;
}

int HeaderView::dividerAt(float x) const noexcept {
    float cur = 0.0f;
    float hitRadius = kDividerHitRadius * dpiScale_;
    for (size_t i = 0; i < sections_.size(); ++i) {
        if (!sections_[i].visible) {
            continue;
        }
        float w = resolvedSectionWidth(i);
        float divX = cur + w;
        if (std::abs(x - divX) <= hitRadius) {
            return static_cast<int>(i);
        }
        cur += w;
    }
    return -1;
}

void HeaderView::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        invalidateSectionCaches();
        markLayoutDirty();
    }
}

void HeaderView::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void HeaderView::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        resolvedDefaultFont_.reset();
        invalidateSectionCaches();
        style().height = layout::Length::points(kDefaultHeaderHeight * dpiScale_);
        markLayoutDirty();
    }
}

void HeaderView::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void HeaderView::onThemeChanged() {
    resolvedDefaultFont_.reset();
    invalidateSectionCaches();
    markLayoutDirty();
    markPaintDirty();
}

void HeaderView::onEnabledChanged() {
    if (!isEnabled()) {
        cancelActiveInteraction();
    }
    markPaintDirty();
}

void HeaderView::invalidateSectionCaches() noexcept {
    sectionCaches_.clear();
}

void HeaderView::ensureSectionLayout(size_t index, float maxWidth) const {
    if (index >= sections_.size()) {
        return;
    }
    if (sectionCaches_.size() != sections_.size()) {
        sectionCaches_.assign(sections_.size(), SectionTextCache{});
    }
    float clampedMax = std::max(0.0f, maxWidth);
    auto& cache = sectionCaches_[index];
    if (cache.valid && std::abs(cache.maxWidth - clampedMax) <= 0.5f) {
        return;
    }
    cache.layout.reset();
    cache.maxWidth = clampedMax;
    auto f = effectiveFont();
    if (f && !sections_[index].title.empty() && clampedMax > 4.0f) {
        text::TextLayoutOptions opts{};
        opts.maxWidth = clampedMax;
        opts.wrapMode = text::WordWrap::None;
        opts.overflow = text::TextOverflow::Ellipsis;
        opts.maxLines = 1;
        cache.layout = text::TextLayout::create(sections_[index].title, f, opts);
    }
    cache.valid = true;
}

std::shared_ptr<text::Font> HeaderView::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float pt = 10.0f * dpiScale_;
        resolvedDefaultFont_ = text::FontManager::instance().defaultFont(pt).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void HeaderView::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();
    float borderW = std::max(1.0f, std::round(1.0f * dpiScale_));
    ctx.drawRect(sRect, pal.surfaceBase);
    ctx.drawLine(core::Point{sRect.origin.x, sRect.origin.y + sRect.size.height - borderW * 0.5f},
                 core::Point{sRect.origin.x + sRect.size.width,
                             sRect.origin.y + sRect.size.height - borderW * 0.5f},
                 pal.borderSubtle, borderW);

    ctx.pushClip(sRect);

    float curX = sRect.origin.x - horizontalOffset_;
    auto f = effectiveFont();
    for (size_t i = 0; i < sections_.size(); ++i) {
        if (!sections_[i].visible) {
            continue;
        }
        float w = resolvedSectionWidth(i);
        core::Rect secRect{core::Point{curX, sRect.origin.y}, {w, sRect.size.height - borderW}};

        if (static_cast<int>(i) == hoveredSection_ && !isDraggingDivider_) {
            ctx.drawRect(secRect, pal.surfaceRaised);
        }

        if (f && !sections_[i].title.empty()) {
            float leftPad = 8.0f * dpiScale_;
            float rightPad = (sections_[i].sortOrder != SortOrder::None) ? (24.0f * dpiScale_)
                                                                         : (8.0f * dpiScale_);
            float maxTitleW = std::max(0.0f, w - leftPad - rightPad);
            ensureSectionLayout(i, maxTitleW);
            if (i < sectionCaches_.size() && sectionCaches_[i].layout) {
                const auto& l = *sectionCaches_[i].layout;
                float textY =
                    theme::computeCenteredTextY(secRect.origin.y + secRect.size.height * 0.5f, *f);
                float textX = secRect.origin.x + leftPad;

                if (sections_[i].alignment == text::TextAlignment::Right) {
                    textX = secRect.origin.x + secRect.size.width - rightPad - l.size().width;
                } else if (sections_[i].alignment == text::TextAlignment::Center) {
                    textX = secRect.origin.x + leftPad +
                            std::max(0.0f, (maxTitleW - l.size().width) * 0.5f);
                }

                core::Color textCol =
                    (sections_[i].sortOrder != SortOrder::None)
                        ? pal.accentDefault
                        : (static_cast<int>(i) == hoveredSection_ ? pal.textPrimary
                                                                  : pal.textSecondary);

                ctx.drawTextLayout(core::Point{textX, textY}, l, textCol);
            }
        }

        if (sections_[i].sortOrder != SortOrder::None) {
            IconId sortIcon = (sections_[i].sortOrder == SortOrder::Ascending)
                                  ? IconId::SortAscending
                                  : IconId::SortDescending;
            if (auto doc = getSvgDocument(sortIcon)) {
                float iconSize = 14.0f * dpiScale_;
                core::Rect arrowRect{
                    core::Point{secRect.origin.x + secRect.size.width - 20.0f * dpiScale_,
                                secRect.origin.y + (secRect.size.height - iconSize) * 0.5f},
                    core::Size{iconSize, iconSize}};
                ctx.drawSvg(*doc, arrowRect, pal.accentDefault);
            }
        }

        if (i + 1 < sections_.size()) {
            bool activeDivider = (hoveredDivider_ == static_cast<int>(i)) ||
                                 (isDraggingDivider_ && dragDivider_ == static_cast<int>(i));
            core::Color divCol =
                activeDivider ? pal.accentDefault.withAlpha(0.75f) : pal.borderSubtle;
            float divW = activeDivider ? 2.0f * dpiScale_ : borderW;
            ctx.drawLine(core::Point{curX + w - borderW * 0.5f, sRect.origin.y + 4.0f * dpiScale_},
                         core::Point{curX + w - borderW * 0.5f,
                                     sRect.origin.y + sRect.size.height - 5.0f * dpiScale_},
                         divCol, divW);
        }

        curX += w;
    }

    ctx.popClip();
}

void HeaderView::cancelActiveInteraction() {
    if (context_ && context_->mouseGrabWidget() == this) {
        context_->releaseMouseGrab();
    }
    bool wasActive =
        isDraggingDivider_ || pressedSection_ >= 0 || hoveredSection_ >= 0 || hoveredDivider_ >= 0;
    isDraggingDivider_ = false;
    dragDivider_ = -1;
    pressedSection_ = -1;
    hoveredSection_ = -1;
    hoveredDivider_ = -1;
    setCursorShape(platform::CursorShape::Arrow);
    if (wasActive) {
        markPaintDirty();
    }
}

void HeaderView::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (isDraggingDivider_ && dragDivider_ >= 0 &&
        dragDivider_ < static_cast<int>(sections_.size())) {
        float delta = ev.position().x - dragStartMouseX_;
        float newW = std::max(sections_[static_cast<size_t>(dragDivider_)].minWidth,
                              dragStartWidth_ + delta);
        if (sections_[static_cast<size_t>(dragDivider_)].width != newW) {
            sections_[static_cast<size_t>(dragDivider_)].width = newW;
            markLayoutDirty();
            onSectionResized.emit(static_cast<size_t>(dragDivider_), newW);
        }
        ev.accept();
        return;
    }

    float localX = ev.position().x + horizontalOffset_;
    int div = dividerAt(localX);
    if (div != hoveredDivider_) {
        hoveredDivider_ = div;
        setCursorShape(hoveredDivider_ >= 0 ? platform::CursorShape::ResizeEW
                                            : platform::CursorShape::Arrow);
        markPaintDirty();
    }

    int sec = sectionAt(localX);
    if (sec != hoveredSection_) {
        hoveredSection_ = sec;
        markPaintDirty();
    }
}

void HeaderView::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.isPrimaryButton()) {
        float localX = ev.position().x + horizontalOffset_;
        int div = dividerAt(localX);
        if (div >= 0 && div < static_cast<int>(sections_.size())) {
            isDraggingDivider_ = true;
            dragDivider_ = div;
            dragStartMouseX_ = ev.position().x;
            dragStartWidth_ = sections_[static_cast<size_t>(div)].width;
            if (context_) {
                context_->setMouseGrabWidget(this);
            }
            ev.accept();
            return;
        }

        int sec = sectionAt(localX);
        if (sec >= 0) {
            pressedSection_ = sec;
            hoveredSection_ = sec;
            if (context_) {
                context_->setMouseGrabWidget(this);
            }
            markPaintDirty();
            ev.accept();
        }
    }
}

void HeaderView::onMouseUp(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        cancelActiveInteraction();
        return;
    }
    if (ev.isPrimaryButton()) {
        if (context_ && context_->mouseGrabWidget() == this) {
            context_->releaseMouseGrab();
        }
        float localX = ev.position().x + horizontalOffset_;
        if (isDraggingDivider_) {
            isDraggingDivider_ = false;
            dragDivider_ = -1;
            int div = dividerAt(localX);
            setCursorShape(div >= 0 ? platform::CursorShape::ResizeEW
                                    : platform::CursorShape::Arrow);
            ev.accept();
            markPaintDirty();
            return;
        }

        int sec = sectionAt(localX);
        int pressed = pressedSection_;
        pressedSection_ = -1;
        if (sec >= 0 && sec == pressed) {
            SortOrder current = sections_[static_cast<size_t>(sec)].sortOrder;
            SortOrder next = (current == SortOrder::None)        ? SortOrder::Ascending
                             : (current == SortOrder::Ascending) ? SortOrder::Descending
                                                                 : SortOrder::None;
            setSectionSortOrder(static_cast<size_t>(sec), next);
            ev.accept();
        } else if (pressed >= 0) {
            hoveredSection_ = sec;
            markPaintDirty();
        }
    }
}

void HeaderView::onMouseLeave() {
    if (!isDraggingDivider_ && pressedSection_ < 0) {
        hoveredSection_ = -1;
        hoveredDivider_ = -1;
        setCursorShape(platform::CursorShape::Arrow);
        markPaintDirty();
    }
}

} // namespace northern::widgets
