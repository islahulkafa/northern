#include "widgets/containers/split_view.hpp"

#include "renderer/paint_context.hpp"
#include "widgets/base/theme.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

namespace {
constexpr float kCollapseAnimDuration = 0.15f;
constexpr float kHoverAnimDuration = 0.12f;
constexpr float kDoubleClickMaxMs = 300.0f;
constexpr float kDoubleClickMaxDistSq = 16.0f;
constexpr float kKeyboardStepPx = 8.0f;
constexpr float kKeyboardLargeStepPx = 32.0f;
constexpr float kGripLength = 24.0f;
constexpr float kGripThickness = 4.0f;
constexpr float kGripCornerRadius = 2.0f;
} // namespace

SplitView::SplitView(SplitOrientation orientation, float splitRatio)
    : orientation_(orientation), splitRatio_(std::clamp(splitRatio, 0.05f, 0.95f)),
      savedRatio_(splitRatio_) {
    setFocusable(true);
    style().width = layout::Length::percent(100.0f);
    style().height = layout::Length::percent(100.0f);
    updatePaneStyles();
}

void SplitView::setOrientation(SplitOrientation orientation) {
    if (orientation_ != orientation) {
        orientation_ = orientation;
        isHoveredSash_ = false;
        setCursorShape(platform::CursorShape::Arrow);
        updatePaneStyles();
        markLayoutDirty();
    }
}

void SplitView::setSizeMode(SplitSizeMode mode) {
    if (sizeMode_ != mode) {
        sizeMode_ = mode;
        updatePaneStyles();
        markLayoutDirty();
    }
}

void SplitView::setSplitRatio(float ratio) {
    float clamped = std::clamp(ratio, 0.01f, 0.99f);
    if (splitRatio_ != clamped) {
        splitRatio_ = clamped;
        savedRatio_ = clamped;
        if (collapseState_ != SplitCollapseState::None) {
            collapseState_ = SplitCollapseState::None;
            collapseAnim_ = core::animation::PropertyAnimation<float>(1.0f);
        }
        updatePaneStyles();
        markLayoutDirty();
        emitSizeSignals();
    }
}

void SplitView::setFixedPaneSize(float size) {
    if (sizeMode_ == SplitSizeMode::FixedSecond) {
        setSecondPaneSize(size);
    } else {
        setFirstPaneSize(size);
    }
}

float SplitView::fixedPaneSize() const noexcept {
    return (sizeMode_ == SplitSizeMode::FixedSecond) ? secondPaneSize_ : firstPaneSize_;
}

void SplitView::setFirstPaneSize(float size) {
    float avail = availableSplitSpan();
    float clamped = (avail > 0.0f) ? clampFirstPaneSize(size, avail)
                                   : std::clamp(size, minFirstPaneSize_, maxFirstPaneSize_);
    if (firstPaneSize_ != clamped) {
        firstPaneSize_ = clamped;
        savedFirstPaneSize_ = clamped;
        if (avail > 0.0f) {
            splitRatio_ = std::clamp(clamped / avail, 0.01f, 0.99f);
            savedRatio_ = splitRatio_;
        }
        if (collapseState_ != SplitCollapseState::None) {
            collapseState_ = SplitCollapseState::None;
            collapseAnim_ = core::animation::PropertyAnimation<float>(1.0f);
        }
        updatePaneStyles();
        markLayoutDirty();
        emitSizeSignals();
    }
}

void SplitView::setSecondPaneSize(float size) {
    float avail = availableSplitSpan();
    float clamped = (avail > 0.0f) ? clampSecondPaneSize(size, avail)
                                   : std::clamp(size, minSecondPaneSize_, maxSecondPaneSize_);
    if (secondPaneSize_ != clamped) {
        secondPaneSize_ = clamped;
        savedSecondPaneSize_ = clamped;
        if (avail > 0.0f) {
            splitRatio_ = std::clamp(1.0f - clamped / avail, 0.01f, 0.99f);
            savedRatio_ = splitRatio_;
        }
        if (collapseState_ != SplitCollapseState::None) {
            collapseState_ = SplitCollapseState::None;
            collapseAnim_ = core::animation::PropertyAnimation<float>(1.0f);
        }
        updatePaneStyles();
        markLayoutDirty();
        emitSizeSignals();
    }
}

void SplitView::setMinPaneSize(float minSize) noexcept {
    float clamped = std::max(0.0f, minSize);
    minFirstPaneSize_ = clamped;
    minSecondPaneSize_ = clamped;
    markLayoutDirty();
}

void SplitView::setMinFirstPaneSize(float minSize) noexcept {
    minFirstPaneSize_ = std::max(0.0f, minSize);
    updatePaneStyles();
    markLayoutDirty();
}

void SplitView::setMaxFirstPaneSize(float maxSize) noexcept {
    maxFirstPaneSize_ = std::max(minFirstPaneSize_, maxSize);
    updatePaneStyles();
    markLayoutDirty();
}

void SplitView::setMinSecondPaneSize(float minSize) noexcept {
    minSecondPaneSize_ = std::max(0.0f, minSize);
    updatePaneStyles();
    markLayoutDirty();
}

void SplitView::setMaxSecondPaneSize(float maxSize) noexcept {
    maxSecondPaneSize_ = std::max(minSecondPaneSize_, maxSize);
    updatePaneStyles();
    markLayoutDirty();
}

void SplitView::setSnapThreshold(float threshold) noexcept {
    snapThreshold_ = std::max(0.0f, threshold);
}

void SplitView::setCollapsible(bool firstCollapsible, bool secondCollapsible) noexcept {
    firstCollapsible_ = firstCollapsible;
    secondCollapsible_ = secondCollapsible;
}

void SplitView::collapseFirstPane(bool animated) {
    if (firstCollapsible_) {
        applyCollapseState(SplitCollapseState::FirstCollapsed, animated);
    }
}

void SplitView::collapseSecondPane(bool animated) {
    if (secondCollapsible_) {
        applyCollapseState(SplitCollapseState::SecondCollapsed, animated);
    }
}

void SplitView::expandPane(bool animated) {
    applyCollapseState(SplitCollapseState::None, animated);
}

void SplitView::toggleCollapse(bool animated) {
    if (collapseState_ != SplitCollapseState::None) {
        expandPane(animated);
    } else if (sizeMode_ == SplitSizeMode::FixedSecond && secondCollapsible_) {
        collapseSecondPane(animated);
    } else if (firstCollapsible_) {
        collapseFirstPane(animated);
    } else if (secondCollapsible_) {
        collapseSecondPane(animated);
    }
}

void SplitView::setSashThickness(float thickness) noexcept {
    float clamped = std::max(2.0f, thickness);
    if (sashThickness_ != clamped) {
        sashThickness_ = clamped;
        updatePaneStyles();
        markLayoutDirty();
    }
}

void SplitView::setGrabMargin(float margin) noexcept {
    grabMargin_ = std::max(0.0f, margin);
}

void SplitView::setFirstPane(std::unique_ptr<Widget> pane) {
    if (firstPane_) {
        removeChild(firstPane_);
        firstPane_ = nullptr;
    }
    if (pane) {
        firstPane_ = pane.get();
        if (secondPane_) {
            auto sec = removeChild(secondPane_);
            addChild(std::move(pane));
            secondPane_ = addChild(std::move(sec));
        } else {
            addChild(std::move(pane));
        }
    }
    updatePaneStyles();
    markLayoutDirty();
}

void SplitView::setSecondPane(std::unique_ptr<Widget> pane) {
    if (secondPane_) {
        removeChild(secondPane_);
        secondPane_ = nullptr;
    }
    if (pane) {
        secondPane_ = addChild(std::move(pane));
    }
    updatePaneStyles();
    markLayoutDirty();
}

void SplitView::setPanes(std::unique_ptr<Widget> first, std::unique_ptr<Widget> second) {
    clearChildren();
    firstPane_ = nullptr;
    secondPane_ = nullptr;
    if (first) {
        firstPane_ = addChild(std::move(first));
    }
    if (second) {
        secondPane_ = addChild(std::move(second));
    }
    updatePaneStyles();
    markLayoutDirty();
}

float SplitView::availableSplitSpan() const noexcept {
    core::Rect lRect = layoutRect();
    float total =
        (orientation_ == SplitOrientation::Horizontal) ? lRect.size.width : lRect.size.height;
    return std::max(0.0f, total - sashThickness_);
}

float SplitView::clampFirstPaneSize(float size, float avail) const noexcept {
    float min1 = minFirstPaneSize_;
    if (std::isfinite(maxSecondPaneSize_)) {
        min1 = std::max(min1, avail - maxSecondPaneSize_);
    }
    float max1 = std::min(maxFirstPaneSize_, std::max(min1, avail - minSecondPaneSize_));
    return std::clamp(size, min1, std::max(min1, max1));
}

float SplitView::clampSecondPaneSize(float size, float avail) const noexcept {
    float min2 = minSecondPaneSize_;
    if (std::isfinite(maxFirstPaneSize_)) {
        min2 = std::max(min2, avail - maxFirstPaneSize_);
    }
    float max2 = std::min(maxSecondPaneSize_, std::max(min2, avail - minFirstPaneSize_));
    return std::clamp(size, min2, std::max(min2, max2));
}

void SplitView::applyCollapseState(SplitCollapseState newState, bool animated) {
    if (collapseState_ == newState) {
        return;
    }

    SplitCollapseState oldState = collapseState_;
    if (oldState == SplitCollapseState::None) {
        savedRatio_ = splitRatio_;
        savedFirstPaneSize_ = firstPaneSize_;
        savedSecondPaneSize_ = secondPaneSize_;
    }

    if (newState == SplitCollapseState::FirstCollapsed) {
        lastCollapsedIsFirst_ = true;
    } else if (newState == SplitCollapseState::SecondCollapsed) {
        lastCollapsedIsFirst_ = false;
    }

    collapseState_ = newState;

    if (newState == SplitCollapseState::None) {
        splitRatio_ = savedRatio_;
        firstPaneSize_ = savedFirstPaneSize_;
        secondPaneSize_ = savedSecondPaneSize_;
    }

    if (firstPane_) {
        firstPane_->setVisible(true);
    }
    if (secondPane_) {
        secondPane_->setVisible(true);
    }

    float target = (newState == SplitCollapseState::None) ? 1.0f : 0.0f;
    if (animated) {
        collapseAnim_.start(collapseAnim_.currentValue(), target, kCollapseAnimDuration,
                            core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
    } else {
        collapseAnim_ = core::animation::PropertyAnimation<float>(target);
    }

    updatePaneStyles();
    markLayoutDirty();
    markPaintDirty();

    if (newState == SplitCollapseState::FirstCollapsed) {
        onCollapseChanged.emit(true, true);
    } else if (newState == SplitCollapseState::SecondCollapsed) {
        onCollapseChanged.emit(false, true);
    } else {
        bool wasFirst = (oldState == SplitCollapseState::FirstCollapsed);
        onCollapseChanged.emit(wasFirst, false);
    }
    emitSizeSignals();
}

void SplitView::emitSizeSignals() {
    onSplitChanged.emit(splitRatio_);

    float avail = availableSplitSpan();
    float s1 = 0.0f;
    float s2 = 0.0f;
    if (collapseState_ == SplitCollapseState::FirstCollapsed) {
        s1 = 0.0f;
        s2 = avail;
    } else if (collapseState_ == SplitCollapseState::SecondCollapsed) {
        s1 = avail;
        s2 = 0.0f;
    } else if (sizeMode_ == SplitSizeMode::FixedFirst) {
        s1 = std::min(firstPaneSize_, avail);
        s2 = std::max(0.0f, avail - s1);
    } else if (sizeMode_ == SplitSizeMode::FixedSecond) {
        s2 = std::min(secondPaneSize_, avail);
        s1 = std::max(0.0f, avail - s2);
    } else {
        s1 = avail * splitRatio_;
        s2 = avail * (1.0f - splitRatio_);
    }
    onPaneResized.emit(s1, s2);
}

void SplitView::updatePaneStyles() {
    float t = collapseAnim_.currentValue();
    bool settledCollapsed =
        (collapseState_ != SplitCollapseState::None) && !collapseAnim_.isRunning() && t <= 0.001f;

    style().padding = layout::RectLength{};

    if (settledCollapsed) {
        if (collapseState_ == SplitCollapseState::FirstCollapsed) {
            if (firstPane_) {
                firstPane_->setVisible(false);
            }
            if (secondPane_) {
                secondPane_->setVisible(true);
                secondPane_->style().flexGrow = 1.0f;
                secondPane_->style().flexShrink = 1.0f;
                if (orientation_ == SplitOrientation::Horizontal) {
                    secondPane_->style().width = layout::Length::points(0.0f);
                    secondPane_->style().height = layout::Length::percent(100.0f);
                } else {
                    secondPane_->style().height = layout::Length::points(0.0f);
                    secondPane_->style().width = layout::Length::percent(100.0f);
                }
            }
            if (orientation_ == SplitOrientation::Horizontal) {
                style().flexDirection = layout::FlexDirection::Row;
                style().padding.left = layout::Length::points(sashThickness_);
                style().columnGap = 0.0f;
                style().rowGap = 0.0f;
            } else {
                style().flexDirection = layout::FlexDirection::Column;
                style().padding.top = layout::Length::points(sashThickness_);
                style().rowGap = 0.0f;
                style().columnGap = 0.0f;
            }
            return;
        }

        if (collapseState_ == SplitCollapseState::SecondCollapsed) {
            if (secondPane_) {
                secondPane_->setVisible(false);
            }
            if (firstPane_) {
                firstPane_->setVisible(true);
                firstPane_->style().flexGrow = 1.0f;
                firstPane_->style().flexShrink = 1.0f;
                if (orientation_ == SplitOrientation::Horizontal) {
                    firstPane_->style().width = layout::Length::points(0.0f);
                    firstPane_->style().height = layout::Length::percent(100.0f);
                } else {
                    firstPane_->style().height = layout::Length::points(0.0f);
                    firstPane_->style().width = layout::Length::percent(100.0f);
                }
            }
            if (orientation_ == SplitOrientation::Horizontal) {
                style().flexDirection = layout::FlexDirection::Row;
                style().padding.right = layout::Length::points(sashThickness_);
                style().columnGap = 0.0f;
                style().rowGap = 0.0f;
            } else {
                style().flexDirection = layout::FlexDirection::Column;
                style().padding.bottom = layout::Length::points(sashThickness_);
                style().rowGap = 0.0f;
                style().columnGap = 0.0f;
            }
            return;
        }
    }

    if (firstPane_) {
        firstPane_->setVisible(true);
    }
    if (secondPane_) {
        secondPane_->setVisible(true);
    }

    bool collapsingFirst =
        (collapseState_ == SplitCollapseState::FirstCollapsed) ||
        (collapseState_ == SplitCollapseState::None && lastCollapsedIsFirst_ && t < 0.999f);
    bool collapsingSecond =
        (collapseState_ == SplitCollapseState::SecondCollapsed) ||
        (collapseState_ == SplitCollapseState::None && !lastCollapsedIsFirst_ && t < 0.999f);

    float effRatio = splitRatio_;
    float effFirstSize = firstPaneSize_;
    float effSecondSize = secondPaneSize_;

    if (collapsingFirst) {
        effRatio = splitRatio_ * t;
        effFirstSize = firstPaneSize_ * t;
    }
    if (collapsingSecond) {
        effRatio = 1.0f - (1.0f - splitRatio_) * t;
        effSecondSize = secondPaneSize_ * t;
    }

    if (orientation_ == SplitOrientation::Horizontal) {
        style().flexDirection = layout::FlexDirection::Row;
        style().columnGap = sashThickness_;
        style().rowGap = 0.0f;

        if (sizeMode_ == SplitSizeMode::FixedFirst) {
            if (firstPane_) {
                firstPane_->style().width = layout::Length::points(effFirstSize);
                firstPane_->style().height = layout::Length::percent(100.0f);
                firstPane_->style().flexGrow = 0.0f;
                firstPane_->style().flexShrink = 0.0f;
            }
            if (secondPane_) {
                secondPane_->style().width = layout::Length::points(0.0f);
                secondPane_->style().height = layout::Length::percent(100.0f);
                secondPane_->style().flexGrow = 1.0f;
                secondPane_->style().flexShrink = 1.0f;
            }
        } else if (sizeMode_ == SplitSizeMode::FixedSecond) {
            if (firstPane_) {
                firstPane_->style().width = layout::Length::points(0.0f);
                firstPane_->style().height = layout::Length::percent(100.0f);
                firstPane_->style().flexGrow = 1.0f;
                firstPane_->style().flexShrink = 1.0f;
            }
            if (secondPane_) {
                secondPane_->style().width = layout::Length::points(effSecondSize);
                secondPane_->style().height = layout::Length::percent(100.0f);
                secondPane_->style().flexGrow = 0.0f;
                secondPane_->style().flexShrink = 0.0f;
            }
        } else {
            if (firstPane_) {
                firstPane_->style().width = layout::Length::points(0.0f);
                firstPane_->style().height = layout::Length::percent(100.0f);
                firstPane_->style().flexGrow = effRatio;
                firstPane_->style().flexShrink = 1.0f;
            }
            if (secondPane_) {
                secondPane_->style().width = layout::Length::points(0.0f);
                secondPane_->style().height = layout::Length::percent(100.0f);
                secondPane_->style().flexGrow = 1.0f - effRatio;
                secondPane_->style().flexShrink = 1.0f;
            }
        }
    } else {
        style().flexDirection = layout::FlexDirection::Column;
        style().rowGap = sashThickness_;
        style().columnGap = 0.0f;

        if (sizeMode_ == SplitSizeMode::FixedFirst) {
            if (firstPane_) {
                firstPane_->style().height = layout::Length::points(effFirstSize);
                firstPane_->style().width = layout::Length::percent(100.0f);
                firstPane_->style().flexGrow = 0.0f;
                firstPane_->style().flexShrink = 0.0f;
            }
            if (secondPane_) {
                secondPane_->style().height = layout::Length::points(0.0f);
                secondPane_->style().width = layout::Length::percent(100.0f);
                secondPane_->style().flexGrow = 1.0f;
                secondPane_->style().flexShrink = 1.0f;
            }
        } else if (sizeMode_ == SplitSizeMode::FixedSecond) {
            if (firstPane_) {
                firstPane_->style().height = layout::Length::points(0.0f);
                firstPane_->style().width = layout::Length::percent(100.0f);
                firstPane_->style().flexGrow = 1.0f;
                firstPane_->style().flexShrink = 1.0f;
            }
            if (secondPane_) {
                secondPane_->style().height = layout::Length::points(effSecondSize);
                secondPane_->style().width = layout::Length::percent(100.0f);
                secondPane_->style().flexGrow = 0.0f;
                secondPane_->style().flexShrink = 0.0f;
            }
        } else {
            if (firstPane_) {
                firstPane_->style().height = layout::Length::points(0.0f);
                firstPane_->style().width = layout::Length::percent(100.0f);
                firstPane_->style().flexGrow = effRatio;
                firstPane_->style().flexShrink = 1.0f;
            }
            if (secondPane_) {
                secondPane_->style().height = layout::Length::points(0.0f);
                secondPane_->style().width = layout::Length::percent(100.0f);
                secondPane_->style().flexGrow = 1.0f - effRatio;
                secondPane_->style().flexShrink = 1.0f;
            }
        }
    }
}

core::Rect SplitView::sashLocalRect() const noexcept {
    core::Rect lRect = layoutRect();
    bool settledCollapsed =
        (collapseState_ != SplitCollapseState::None) && !collapseAnim_.isRunning();

    if (orientation_ == SplitOrientation::Horizontal) {
        float x = 0.0f;
        if (settledCollapsed && collapseState_ == SplitCollapseState::FirstCollapsed) {
            x = 0.0f;
        } else if (settledCollapsed && collapseState_ == SplitCollapseState::SecondCollapsed) {
            x = std::max(0.0f, lRect.size.width - sashThickness_);
        } else if (firstPane_) {
            x = firstPane_->layoutRect().origin.x + firstPane_->layoutRect().size.width;
        } else {
            x = lRect.size.width * splitRatio_;
        }
        return core::Rect{core::Point{x, 0.0f}, core::Size{sashThickness_, lRect.size.height}};
    } else {
        float y = 0.0f;
        if (settledCollapsed && collapseState_ == SplitCollapseState::FirstCollapsed) {
            y = 0.0f;
        } else if (settledCollapsed && collapseState_ == SplitCollapseState::SecondCollapsed) {
            y = std::max(0.0f, lRect.size.height - sashThickness_);
        } else if (firstPane_) {
            y = firstPane_->layoutRect().origin.y + firstPane_->layoutRect().size.height;
        } else {
            y = lRect.size.height * splitRatio_;
        }
        return core::Rect{core::Point{0.0f, y}, core::Size{lRect.size.width, sashThickness_}};
    }
}

core::Rect SplitView::sashHitRect() const noexcept {
    core::Rect local = sashLocalRect();
    core::Rect lRect = layoutRect();
    if (orientation_ == SplitOrientation::Horizontal) {
        float left = std::max(0.0f, local.origin.x - grabMargin_);
        float right = std::min(lRect.size.width, local.origin.x + local.size.width + grabMargin_);
        return core::Rect{core::Point{left, local.origin.y},
                          core::Size{std::max(0.0f, right - left), local.size.height}};
    } else {
        float top = std::max(0.0f, local.origin.y - grabMargin_);
        float bottom =
            std::min(lRect.size.height, local.origin.y + local.size.height + grabMargin_);
        return core::Rect{core::Point{local.origin.x, top},
                          core::Size{local.size.width, std::max(0.0f, bottom - top)}};
    }
}

core::Rect SplitView::sashScreenRect() const noexcept {
    core::Rect sRect = screenRect();
    core::Rect local = sashLocalRect();
    return core::Rect{core::Point{sRect.origin.x + local.origin.x, sRect.origin.y + local.origin.y},
                      local.size};
}

core::Rect SplitView::gripScreenRect() const noexcept {
    core::Rect sRect = sashScreenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return core::Rect{};
    }
    if (orientation_ == SplitOrientation::Horizontal) {
        float gripH = std::min(kGripLength, sRect.size.height * 0.5f);
        float gx = sRect.origin.x + (sRect.size.width - kGripThickness) * 0.5f;
        float gy = sRect.origin.y + (sRect.size.height - gripH) * 0.5f;
        return core::Rect{core::Point{gx, gy}, core::Size{kGripThickness, gripH}};
    } else {
        float gripW = std::min(kGripLength, sRect.size.width * 0.5f);
        float gx = sRect.origin.x + (sRect.size.width - gripW) * 0.5f;
        float gy = sRect.origin.y + (sRect.size.height - kGripThickness) * 0.5f;
        return core::Rect{core::Point{gx, gy}, core::Size{gripW, kGripThickness}};
    }
}

void SplitView::nudgeSplit(float deltaPixels) {
    float avail = availableSplitSpan();
    if (avail <= 0.0f) {
        return;
    }

    if (collapseState_ != SplitCollapseState::None) {
        expandPane(false);
    }

    if (sizeMode_ == SplitSizeMode::FixedFirst) {
        setFirstPaneSize(firstPaneSize_ + deltaPixels);
    } else if (sizeMode_ == SplitSizeMode::FixedSecond) {
        setSecondPaneSize(secondPaneSize_ - deltaPixels);
    } else {
        float curFirst = avail * splitRatio_;
        float clampedFirst = clampFirstPaneSize(curFirst + deltaPixels, avail);
        setSplitRatio(clampedFirst / avail);
    }
}

void SplitView::onMouseMove(WidgetMouseEvent& ev) {
    core::Rect hitRect = sashHitRect();

    if (isDraggingSash_) {
        float avail = availableSplitSpan();
        if (avail > 0.0f) {
            float pos =
                (orientation_ == SplitOrientation::Horizontal) ? ev.position().x : ev.position().y;
            float candidateFirst = pos - sashThickness_ * 0.5f;
            float candidateSecond = avail - candidateFirst;

            if (firstCollapsible_ && candidateFirst < snapThreshold_) {
                applyCollapseState(SplitCollapseState::FirstCollapsed, false);
            } else if (secondCollapsible_ && candidateSecond < snapThreshold_) {
                applyCollapseState(SplitCollapseState::SecondCollapsed, false);
            } else {
                if (collapseState_ != SplitCollapseState::None) {
                    bool wasFirst = (collapseState_ == SplitCollapseState::FirstCollapsed);
                    collapseState_ = SplitCollapseState::None;
                    collapseAnim_ = core::animation::PropertyAnimation<float>(1.0f);
                    onCollapseChanged.emit(wasFirst, false);
                }

                if (sizeMode_ == SplitSizeMode::FixedFirst) {
                    setFirstPaneSize(candidateFirst);
                } else if (sizeMode_ == SplitSizeMode::FixedSecond) {
                    setSecondPaneSize(candidateSecond);
                } else {
                    float clampedFirst = clampFirstPaneSize(candidateFirst, avail);
                    setSplitRatio(clampedFirst / avail);
                }
            }
        }
        ev.accept();
        return;
    }

    bool inside = hitRect.contains(ev.position());
    if (inside != isHoveredSash_) {
        isHoveredSash_ = inside;
        setCursorShape(isHoveredSash_ ? (orientation_ == SplitOrientation::Horizontal
                                             ? platform::CursorShape::ResizeEW
                                             : platform::CursorShape::ResizeNS)
                                      : platform::CursorShape::Arrow);
        hoverAnim_.start(hoverAnim_.currentValue(), isHoveredSash_ ? 1.0f : 0.0f,
                         kHoverAnimDuration, core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
    } else if (isHoveredSash_) {
        platform::CursorShape expected =
            (orientation_ == SplitOrientation::Horizontal ? platform::CursorShape::ResizeEW
                                                          : platform::CursorShape::ResizeNS);
        if (cursorShape() != expected) {
            setCursorShape(expected);
        }
    }
}

void SplitView::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        core::Rect hitRect = sashHitRect();
        if (hitRect.contains(ev.position())) {
            auto now = std::chrono::steady_clock::now();
            auto elapsedMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - lastClickTime_).count();
            float dx = ev.position().x - lastClickPos_.x;
            float dy = ev.position().y - lastClickPos_.y;
            float distSq = dx * dx + dy * dy;

            bool isDoubleClick =
                (elapsedMs > 0 && elapsedMs < kDoubleClickMaxMs && distSq <= kDoubleClickMaxDistSq);
            lastClickTime_ = now;
            lastClickPos_ = ev.position();

            if (isFocusable()) {
                setFocus(FocusReason::Pointer);
            }

            if (isDoubleClick) {
                toggleCollapse(true);
                ev.accept();
                return;
            }

            isDraggingSash_ = true;
            if (context_) {
                context_->setMouseGrabWidget(this);
            }
            ev.accept();
            markPaintDirty();
        }
    }
}

void SplitView::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && isDraggingSash_) {
        isDraggingSash_ = false;
        if (context_) {
            context_->releaseMouseGrab();
        }
        core::Rect hitRect = sashHitRect();
        if (!hitRect.contains(ev.position())) {
            isHoveredSash_ = false;
            setCursorShape(platform::CursorShape::Arrow);
            hoverAnim_.start(hoverAnim_.currentValue(), 0.0f, kHoverAnimDuration,
                             core::animation::EasingCurve::OutCubic);
            if (context_) {
                context_->registerAnimatedWidget(this);
            }
        }
        ev.accept();
        markPaintDirty();
    }
}

void SplitView::onMouseLeave() {
    if (!isDraggingSash_ && isHoveredSash_) {
        isHoveredSash_ = false;
        setCursorShape(platform::CursorShape::Arrow);
        hoverAnim_.start(hoverAnim_.currentValue(), 0.0f, kHoverAnimDuration,
                         core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
        markPaintDirty();
    }
}

void SplitView::onKeyDown(WidgetKeyEvent& ev) {
    if (!ev.isDown()) {
        return;
    }

    float step = platform::hasModifier(ev.modifiers(), platform::KeyModifier::Shift)
                     ? kKeyboardLargeStepPx
                     : kKeyboardStepPx;
    float avail = availableSplitSpan();

    switch (ev.key()) {
    case platform::Key::Left:
        if (orientation_ == SplitOrientation::Horizontal) {
            nudgeSplit(-step);
            ev.accept();
        }
        break;
    case platform::Key::Right:
        if (orientation_ == SplitOrientation::Horizontal) {
            nudgeSplit(step);
            ev.accept();
        }
        break;
    case platform::Key::Up:
        if (orientation_ == SplitOrientation::Vertical) {
            nudgeSplit(-step);
            ev.accept();
        }
        break;
    case platform::Key::Down:
        if (orientation_ == SplitOrientation::Vertical) {
            nudgeSplit(step);
            ev.accept();
        }
        break;
    case platform::Key::Home:
        if (avail > 0.0f) {
            if (collapseState_ != SplitCollapseState::None) {
                expandPane(false);
            }
            if (sizeMode_ == SplitSizeMode::FixedFirst) {
                setFirstPaneSize(minFirstPaneSize_);
            } else if (sizeMode_ == SplitSizeMode::FixedSecond) {
                setSecondPaneSize(clampSecondPaneSize(avail - minFirstPaneSize_, avail));
            } else {
                setSplitRatio(clampFirstPaneSize(minFirstPaneSize_, avail) / avail);
            }
            ev.accept();
        }
        break;
    case platform::Key::End:
        if (avail > 0.0f) {
            if (collapseState_ != SplitCollapseState::None) {
                expandPane(false);
            }
            if (sizeMode_ == SplitSizeMode::FixedFirst) {
                setFirstPaneSize(clampFirstPaneSize(avail - minSecondPaneSize_, avail));
            } else if (sizeMode_ == SplitSizeMode::FixedSecond) {
                setSecondPaneSize(minSecondPaneSize_);
            } else {
                setSplitRatio(clampFirstPaneSize(avail - minSecondPaneSize_, avail) / avail);
            }
            ev.accept();
        }
        break;
    case platform::Key::Enter:
    case platform::Key::Space:
        toggleCollapse(true);
        ev.accept();
        break;
    default:
        break;
    }
}

void SplitView::onFocusGained() {
    markPaintDirty();
}

void SplitView::onFocusLost() {
    markPaintDirty();
}

void SplitView::updateAnimations(float deltaSeconds) {
    bool changed = false;
    if (collapseAnim_.isRunning()) {
        collapseAnim_.update(deltaSeconds);
        updatePaneStyles();
        markLayoutDirty();
        changed = true;
    }
    if (hoverAnim_.isRunning()) {
        hoverAnim_.update(deltaSeconds);
        changed = true;
    }
    if (changed) {
        markPaintDirty();
    }
}

bool SplitView::hasActiveAnimations() const noexcept {
    return collapseAnim_.isRunning() || hoverAnim_.isRunning();
}

void SplitView::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = sashScreenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();
    float hoverT = hoverAnim_.currentValue();

    if (isDraggingSash_) {
        ctx.drawRect(sRect, pal.accentDefault.withAlpha(0.14f));
    } else if (hoverT > 0.001f) {
        ctx.drawRect(sRect, pal.surfaceRaised.withAlpha(0.22f * hoverT));
    }

    core::Color lineCol = pal.borderSubtle.lerp(pal.borderStrong, hoverT);
    if (isDraggingSash_) {
        lineCol = pal.accentDefault;
    }

    if (orientation_ == SplitOrientation::Horizontal) {
        float midX = sRect.origin.x + sRect.size.width * 0.5f;
        ctx.drawLine(core::Point{midX, sRect.origin.y},
                     core::Point{midX, sRect.origin.y + sRect.size.height}, lineCol, 1.0f);
    } else {
        float midY = sRect.origin.y + sRect.size.height * 0.5f;
        ctx.drawLine(core::Point{sRect.origin.x, midY},
                     core::Point{sRect.origin.x + sRect.size.width, midY}, lineCol, 1.0f);
    }

    core::Rect gripRect = gripScreenRect();
    if (gripRect.size.width > 0.0f && gripRect.size.height > 0.0f) {
        core::Color gripCol = pal.borderDefault.lerp(pal.borderStrong, hoverT);
        if (isDraggingSash_) {
            gripCol = pal.accentDefault;
        }
        ctx.drawRoundedRect(gripRect, gripCol, kGripCornerRadius);

        if (hasKeyboardFocus()) {
            core::Rect focusRect{
                core::Point{gripRect.origin.x - 2.0f, gripRect.origin.y - 2.0f},
                core::Size{gripRect.size.width + 4.0f, gripRect.size.height + 4.0f}};
            ctx.drawBorder(focusRect, pal.accentFocusRing, 1.5f, kGripCornerRadius + 2.0f);
        }
    }
}

} // namespace northern::widgets
