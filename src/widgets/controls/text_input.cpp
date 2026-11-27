#include "widgets/controls/text_input.hpp"
#include "platform/clipboard.hpp"
#include "renderer/paint_context.hpp"
#include "renderer/svg/svg_document.hpp"
#include "text/font_manager.hpp"
#include "widgets/base/widget_context.hpp"
#include "widgets/controls/icons.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

namespace {

std::string codepointToUtf8(uint32_t cp) {
    std::string s;
    if (cp <= 0x7F) {
        s.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        s.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        s.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        s.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        s.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return s;
}

size_t prevUtf8Char(std::string_view str, size_t idx) {
    if (idx == 0 || idx > str.size()) {
        return 0;
    }
    --idx;
    while (idx > 0 && (static_cast<unsigned char>(str[idx]) & 0xC0) == 0x80) {
        --idx;
    }
    return idx;
}

size_t nextUtf8Char(std::string_view str, size_t idx) {
    if (idx >= str.size()) {
        return str.size();
    }
    ++idx;
    while (idx < str.size() && (static_cast<unsigned char>(str[idx]) & 0xC0) == 0x80) {
        ++idx;
    }
    return idx;
}

size_t utf8CodepointCount(std::string_view str) noexcept {
    size_t count = 0;
    for (unsigned char c : str) {
        if ((c & 0xC0) != 0x80) {
            ++count;
        }
    }
    return count;
}

std::string_view truncateToCodepoints(std::string_view str, size_t maxChars) noexcept {
    size_t count = 0;
    size_t i = 0;
    while (i < str.size()) {
        if (count >= maxChars) {
            return str.substr(0, i);
        }
        i = nextUtf8Char(str, i);
        ++count;
    }
    return str;
}

bool isWhitespaceAscii(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

size_t prevWordBoundary(std::string_view str, size_t idx) {
    if (idx == 0 || idx > str.size()) {
        return 0;
    }
    while (idx > 0) {
        size_t prev = prevUtf8Char(str, idx);
        if (!isWhitespaceAscii(str[prev])) {
            break;
        }
        idx = prev;
    }
    while (idx > 0) {
        size_t prev = prevUtf8Char(str, idx);
        if (isWhitespaceAscii(str[prev])) {
            break;
        }
        idx = prev;
    }
    return idx;
}

size_t nextWordBoundary(std::string_view str, size_t idx) {
    if (idx >= str.size()) {
        return str.size();
    }
    while (idx < str.size() && !isWhitespaceAscii(str[idx])) {
        idx = nextUtf8Char(str, idx);
    }
    while (idx < str.size() && isWhitespaceAscii(str[idx])) {
        idx = nextUtf8Char(str, idx);
    }
    return idx;
}

} // namespace

TextInput::TextInput(std::string placeholder, std::shared_ptr<text::Font> font)
    : placeholder_(std::move(placeholder)), font_(std::move(font)) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::IBeam);
    style().height = layout::Length::points(theme::Metrics::ControlHeightStandard);
    style().padding = layout::RectLength{
        .left = layout::Length::points(10.0f),
        .top = layout::Length::points(0.0f),
        .right = layout::Length::points(10.0f),
        .bottom = layout::Length::points(0.0f),
    };

    layoutNode_.setMeasureFunc([this](const layout::LayoutNode&, float availW,
                                      float) -> core::Size {
        float textW = 0.0f;
        float textH = 20.0f * dpiScale_;
        auto f = effectiveFont();
        if (f) {
            if (!text_.empty()) {
                ensureTextCache();
                if (cachedTextLayout_) {
                    textW = cachedTextLayout_->size().width;
                    textH = std::max(textH, cachedTextLayout_->size().height);
                }
            } else if (!placeholder_.empty()) {
                ensurePlaceholderCache();
                if (cachedPlaceholderLayout_) {
                    textW = cachedPlaceholderLayout_->size().width;
                    textH = std::max(textH, cachedPlaceholderLayout_->size().height);
                }
            }
        }
        float padH = style().padding.left.resolve(availW, 0.0f) +
                     style().padding.right.resolve(availW, 0.0f);
        float padV =
            style().padding.top.resolve(0.0f, 0.0f) + style().padding.bottom.resolve(0.0f, 0.0f);
        return core::Size{std::max(120.0f * dpiScale_, textW + padH), textH + padV};
    });
}

TextInput::TextInput(std::string placeholder, TextInputSize size)
    : TextInput(std::move(placeholder), nullptr) {
    setSize(size);
}

std::shared_ptr<text::Font> TextInput::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float basePt = 10.5f;
        switch (size_) {
        case TextInputSize::Compact:
            basePt = 9.5f;
            break;
        case TextInputSize::Comfortable:
            basePt = 11.5f;
            break;
        case TextInputSize::Standard:
        default:
            basePt = 10.5f;
            break;
        }
        float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
        resolvedDefaultFont_ =
            text::FontManager::instance().defaultFont(basePt * scale).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void TextInput::invalidateTextCache() noexcept {
    textCacheValid_ = false;
    cachedTextLayout_.reset();
    clusterPenX_.clear();
}

void TextInput::ensureTextCache() const {
    if (textCacheValid_) {
        return;
    }
    textCacheValid_ = true;
    clusterPenX_.assign(text_.size() + 1, 0.0f);
    cachedTextLayout_.reset();

    auto f = effectiveFont();
    if (!f || text_.empty()) {
        return;
    }

    if (passwordMode_) {
        size_t cpCount = utf8CodepointCount(text_);
        std::string masked;
        masked.reserve(cpCount * 3);
        for (size_t i = 0; i < cpCount; ++i) {
            masked.append("\xE2\x80\xA2");
        }
        cachedTextLayout_ = text::TextLayout::create(masked, f);
        float totalW = cachedTextLayout_->size().width;
        float bulletStep = (cpCount > 0) ? (totalW / static_cast<float>(cpCount)) : 0.0f;

        size_t cpIdx = 0;
        size_t byteIdx = 0;
        while (byteIdx < text_.size()) {
            size_t nextByte = nextUtf8Char(text_, byteIdx);
            float startX = static_cast<float>(cpIdx) * bulletStep;
            for (size_t b = byteIdx; b < nextByte; ++b) {
                clusterPenX_[b] = startX;
            }
            byteIdx = nextByte;
            ++cpIdx;
        }
        clusterPenX_[text_.size()] = totalW;
        return;
    }

    cachedTextLayout_ = text::TextLayout::create(text_, f);
    if (cachedTextLayout_->lines().empty()) {
        return;
    }

    const auto& glyphs = cachedTextLayout_->lines()[0].glyphs;
    float totalW = cachedTextLayout_->size().width;
    if (glyphs.empty()) {
        std::fill(clusterPenX_.begin(), clusterPenX_.end(), 0.0f);
        return;
    }

    std::vector<bool> hasCluster(text_.size() + 1, false);
    float pen = 0.0f;
    for (const auto& g : glyphs) {
        size_t c = std::min(static_cast<size_t>(g.cluster), text_.size());
        if (!hasCluster[c]) {
            clusterPenX_[c] = pen;
            hasCluster[c] = true;
        }
        pen += g.advance.x;
    }
    clusterPenX_[text_.size()] = totalW;
    hasCluster[text_.size()] = true;

    // Fill intermediate UTF-8 continuation bytes or ligature clusters cleanly
    float lastPen = 0.0f;
    for (size_t i = 0; i <= text_.size(); ++i) {
        if (hasCluster[i]) {
            lastPen = clusterPenX_[i];
        } else {
            clusterPenX_[i] = lastPen;
        }
    }
}

void TextInput::ensurePlaceholderCache() const {
    if (placeholderCacheValid_) {
        return;
    }
    placeholderCacheValid_ = true;
    cachedPlaceholderLayout_.reset();

    auto f = effectiveFont();
    if (!f || placeholder_.empty()) {
        return;
    }
    cachedPlaceholderLayout_ = text::TextLayout::create(placeholder_, f);
}

float TextInput::measureOffsetAt(size_t byteIndex) const {
    if (text_.empty()) {
        return 0.0f;
    }
    ensureTextCache();
    if (clusterPenX_.empty()) {
        return 0.0f;
    }
    size_t clamped = std::min(byteIndex, clusterPenX_.size() - 1);
    return clusterPenX_[clamped];
}

void TextInput::setText(std::string text) {
    if (maxLength_ > 0 && utf8CodepointCount(text) > maxLength_) {
        text = std::string(truncateToCodepoints(text, maxLength_));
    }
    if (text_ != text) {
        text_ = std::move(text);
        cursorIndex_ = std::min(cursorIndex_, text_.size());
        selectionAnchor_ = cursorIndex_;
        invalidateTextCache();
        ensureCursorVisible();
        markPaintDirty();
        onTextChanged.emit(text_);
    }
}

void TextInput::setPlaceholder(std::string placeholder) {
    if (placeholder_ != placeholder) {
        placeholder_ = std::move(placeholder);
        placeholderCacheValid_ = false;
        cachedPlaceholderLayout_.reset();
        markPaintDirty();
    }
}

void TextInput::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        invalidateTextCache();
        placeholderCacheValid_ = false;
        cachedPlaceholderLayout_.reset();
        markLayoutDirty();
    }
}

void TextInput::setSize(TextInputSize size) noexcept {
    if (size_ != size) {
        size_ = size;
        resolvedDefaultFont_.reset();
        if (!font_) {
            invalidateTextCache();
            placeholderCacheValid_ = false;
            cachedPlaceholderLayout_.reset();
        }
    }
    float h = theme::Metrics::ControlHeightStandard;
    switch (size) {
    case TextInputSize::Compact:
        h = theme::Metrics::ControlHeightCompact;
        break;
    case TextInputSize::Comfortable:
        h = theme::Metrics::ControlHeightComfortable;
        break;
    case TextInputSize::Standard:
    default:
        h = theme::Metrics::ControlHeightStandard;
        break;
    }
    style().height = layout::Length::points(h * dpiScale_);
    markLayoutDirty();
}

void TextInput::setDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        float ratio = clamped / dpiScale_;
        dpiScale_ = clamped;
        if (style().height.isPoints()) {
            style().height = layout::Length::points(style().height.value * ratio);
        }
        style().padding.left = layout::Length::points(10.0f * dpiScale_);
        style().padding.right = layout::Length::points(10.0f * dpiScale_);
        resolvedDefaultFont_.reset();
        if (!font_) {
            invalidateTextCache();
            placeholderCacheValid_ = false;
            cachedPlaceholderLayout_.reset();
        }
        markLayoutDirty();
    }
}

void TextInput::setReadOnly(bool readOnly) noexcept {
    if (readOnly_ != readOnly) {
        readOnly_ = readOnly;
        markPaintDirty();
    }
}

void TextInput::setPasswordMode(bool enabled) {
    if (passwordMode_ != enabled) {
        passwordMode_ = enabled;
        invalidateTextCache();
        ensureCursorVisible();
        markPaintDirty();
    }
}

void TextInput::setMaxLength(size_t maxChars) {
    if (maxLength_ != maxChars) {
        maxLength_ = maxChars;
        if (maxLength_ > 0 && utf8CodepointCount(text_) > maxLength_) {
            setText(std::string(truncateToCodepoints(text_, maxLength_)));
        }
    }
}

void TextInput::setLeadingIcon(std::shared_ptr<renderer::SvgDocument> icon) {
    if (leadingIcon_ != icon) {
        leadingIcon_ = std::move(icon);
        ensureCursorVisible();
        markLayoutDirty();
        markPaintDirty();
    }
}

void TextInput::setLeadingIcon(IconId iconId) {
    setLeadingIcon(getSvgDocument(iconId));
}

void TextInput::setClearButtonEnabled(bool enabled) noexcept {
    if (clearButtonEnabled_ != enabled) {
        clearButtonEnabled_ = enabled;
        isHoveringClear_ = false;
        ensureCursorVisible();
        markLayoutDirty();
        markPaintDirty();
    }
}

bool TextInput::shouldShowClearButton() const noexcept {
    return clearButtonEnabled_ && isEnabled() && !readOnly_ && !text_.empty();
}

float TextInput::effectiveTextLeftInset(float availWidth) const noexcept {
    float padL = style().padding.left.resolve(availWidth, 0.0f);
    if (leadingIcon_ && leadingIcon_->isValid()) {
        padL += 14.0f * dpiScale_ + 6.0f * dpiScale_;
    }
    return padL;
}

float TextInput::effectiveTextRightInset(float availWidth) const noexcept {
    float padR = style().padding.right.resolve(availWidth, 0.0f);
    if (clearButtonEnabled_) {
        padR += 14.0f * dpiScale_ + 6.0f * dpiScale_;
    }
    return padR;
}

core::Rect TextInput::clearButtonLocalRect() const noexcept {
    core::Rect r = layoutRect();
    if (!shouldShowClearButton() || r.size.width <= 0.0f || r.size.height <= 0.0f) {
        return {};
    }
    float btnSize = 18.0f * dpiScale_;
    float padR = style().padding.right.resolve(r.size.width, 0.0f);
    float x = r.size.width - padR - btnSize + 2.0f * dpiScale_;
    float y = (r.size.height - btnSize) * 0.5f;
    return core::Rect{core::Point{x, y}, core::Size{btnSize, btnSize}};
}

void TextInput::updateBorderAnimation(bool animated, bool focused) {
    float targetHover = (isEnabled() && isHovered_) ? 1.0f : 0.0f;
    float targetFocus = (isEnabled() && focused) ? 1.0f : 0.0f;
    if (animated && isEnabled()) {
        if (std::abs(hoverAnim_.targetValue() - targetHover) > 0.001f ||
            (!hoverAnim_.isRunning() &&
             std::abs(hoverAnim_.currentValue() - targetHover) > 0.001f)) {
            hoverAnim_.start(hoverAnim_.currentValue(), targetHover, 0.12f,
                             core::animation::EasingCurve::OutCubic);
        }
        if (std::abs(focusAnim_.targetValue() - targetFocus) > 0.001f ||
            (!focusAnim_.isRunning() &&
             std::abs(focusAnim_.currentValue() - targetFocus) > 0.001f)) {
            focusAnim_.start(focusAnim_.currentValue(), targetFocus, 0.14f,
                             core::animation::EasingCurve::OutCubic);
        }
        if (context_ && (hoverAnim_.isRunning() || focusAnim_.isRunning())) {
            context_->registerAnimatedWidget(this);
        }
    } else {
        hoverAnim_.setCurrentValue(targetHover);
        focusAnim_.setCurrentValue(targetFocus);
    }
    markPaintDirty();
}

std::string TextInput::selectedText() const {
    if (!hasSelection()) {
        return "";
    }
    size_t start = std::min(cursorIndex_, selectionAnchor_);
    size_t end = std::max(cursorIndex_, selectionAnchor_);
    return text_.substr(start, end - start);
}

void TextInput::selectAll() {
    selectionAnchor_ = 0;
    cursorIndex_ = text_.size();
    ensureCursorVisible();
    markPaintDirty();
}

void TextInput::setCursorIndex(size_t index, bool keepSelection) {
    cursorIndex_ = std::min(index, text_.size());
    if (!keepSelection) {
        selectionAnchor_ = cursorIndex_;
    }
    ensureCursorVisible();
    markPaintDirty();
}

void TextInput::setBackgroundColor(const core::Color& color) noexcept {
    overrideBgColor_ = color;
    markPaintDirty();
}

void TextInput::setIdleBorderColor(const core::Color& color) noexcept {
    overrideIdleBorderColor_ = color;
    markPaintDirty();
}

void TextInput::setFocusedBorderColor(const core::Color& color) noexcept {
    overrideFocusedBorderColor_ = color;
    markPaintDirty();
}

void TextInput::setTextColor(const core::Color& color) noexcept {
    overrideTextColor_ = color;
    markPaintDirty();
}

void TextInput::setPlaceholderColor(const core::Color& color) noexcept {
    overridePlaceholderColor_ = color;
    markPaintDirty();
}

void TextInput::setSelectionColor(const core::Color& color) noexcept {
    overrideSelectionColor_ = color;
    markPaintDirty();
}

void TextInput::setCursorColor(const core::Color& color) noexcept {
    overrideCursorColor_ = color;
    markPaintDirty();
}

void TextInput::setBorderWidth(float width) noexcept {
    borderWidth_ = width;
    markPaintDirty();
}

void TextInput::setCornerRadius(float radius) noexcept {
    cornerRadii_[0] = cornerRadii_[1] = cornerRadii_[2] = cornerRadii_[3] = radius;
    markPaintDirty();
}

void TextInput::clearColorOverrides() noexcept {
    if (overrideBgColor_ || overrideIdleBorderColor_ || overrideFocusedBorderColor_ ||
        overrideTextColor_ || overridePlaceholderColor_ || overrideSelectionColor_ ||
        overrideCursorColor_) {
        overrideBgColor_.reset();
        overrideIdleBorderColor_.reset();
        overrideFocusedBorderColor_.reset();
        overrideTextColor_.reset();
        overridePlaceholderColor_.reset();
        overrideSelectionColor_.reset();
        overrideCursorColor_.reset();
        markPaintDirty();
    }
}

core::Color TextInput::effectiveBackgroundColor() const noexcept {
    if (overrideBgColor_) {
        return *overrideBgColor_;
    }
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.surfaceBase;
    }
    return (pal.mode == theme::ThemeMode::Dark) ? pal.surfaceBase : pal.surfaceDefault;
}

core::Color TextInput::effectiveBorderColor() const noexcept {
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.borderSubtle;
    }
    if (hasFocus()) {
        return overrideFocusedBorderColor_.value_or(pal.accentDefault);
    }
    if (isHovered_) {
        return pal.borderStrong;
    }
    return overrideIdleBorderColor_.value_or(pal.borderDefault);
}

core::Color TextInput::effectiveTextColor() const noexcept {
    if (overrideTextColor_) {
        return isEnabled() ? *overrideTextColor_
                           : overrideTextColor_->withAlpha(overrideTextColor_->a * 0.40f);
    }
    const auto& pal = colorPalette();
    return isEnabled() ? pal.textPrimary : pal.textDisabled;
}

core::Color TextInput::effectivePlaceholderColor() const noexcept {
    if (overridePlaceholderColor_) {
        return *overridePlaceholderColor_;
    }
    return colorPalette().textTertiary;
}

core::Color TextInput::effectiveSelectionColor() const noexcept {
    if (overrideSelectionColor_) {
        return *overrideSelectionColor_;
    }
    return colorPalette().accentMuted;
}

core::Color TextInput::effectiveCursorColor() const noexcept {
    if (overrideCursorColor_) {
        return *overrideCursorColor_;
    }
    return colorPalette().textPrimary;
}

void TextInput::deleteSelection() {
    if (!hasSelection() || readOnly_) {
        return;
    }
    size_t start = std::min(cursorIndex_, selectionAnchor_);
    size_t end = std::max(cursorIndex_, selectionAnchor_);
    text_.erase(start, end - start);
    cursorIndex_ = start;
    selectionAnchor_ = cursorIndex_;
    cursorBlinkTimer_ = 0.0f;
    cursorVisible_ = true;
    invalidateTextCache();
    ensureCursorVisible();
    markPaintDirty();
    onTextChanged.emit(text_);
}

void TextInput::insertText(std::string_view str) {
    if (str.empty() || readOnly_) {
        return;
    }
    if (hasSelection()) {
        deleteSelection();
    }
    if (maxLength_ > 0) {
        size_t currentChars = utf8CodepointCount(text_);
        if (currentChars >= maxLength_) {
            return;
        }
        size_t budget = maxLength_ - currentChars;
        str = truncateToCodepoints(str, budget);
        if (str.empty()) {
            return;
        }
    }
    text_.insert(cursorIndex_, str);
    cursorIndex_ += str.size();
    selectionAnchor_ = cursorIndex_;
    cursorBlinkTimer_ = 0.0f;
    cursorVisible_ = true;
    invalidateTextCache();
    ensureCursorVisible();
    markPaintDirty();
    onTextChanged.emit(text_);
}

size_t TextInput::hitTestCharacter(float localX) const {
    if (text_.empty() || !effectiveFont()) {
        return 0;
    }
    ensureTextCache();
    float adjustedX = localX + scrollOffset_;
    if (adjustedX <= 0.0f) {
        return 0;
    }

    size_t bestIdx = 0;
    float prevW = 0.0f;
    size_t i = 0;
    while (i <= text_.size()) {
        float w = measureOffsetAt(i);
        if (adjustedX < w) {
            float mid = (prevW + w) * 0.5f;
            return (adjustedX >= mid) ? i : bestIdx;
        }
        bestIdx = i;
        prevW = w;
        if (i == text_.size()) {
            break;
        }
        i = nextUtf8Char(text_, i);
    }
    return text_.size();
}

void TextInput::ensureCursorVisible() {
    if (!effectiveFont()) {
        return;
    }
    float cursorX = measureOffsetAt(cursorIndex_);
    float totalW = layoutRect().size.width;
    float contentW = totalW > 0.0f ? std::max(0.0f, totalW - effectiveTextLeftInset(totalW) -
                                                        effectiveTextRightInset(totalW))
                                   : layoutNode_.contentRect().size.width;
    if (contentW <= 0.0f) {
        return;
    }

    if (cursorX - scrollOffset_ < 0.0f) {
        scrollOffset_ = cursorX;
    } else if (cursorX - scrollOffset_ > contentW) {
        scrollOffset_ = cursorX - contentW + 4.0f;
    }
    scrollOffset_ = std::max(0.0f, scrollOffset_);
}

void TextInput::onMouseEnter() {
    if (!isEnabled()) {
        return;
    }
    isHovered_ = true;
    updateBorderAnimation(true, hasFocus());
}

void TextInput::onMouseLeave() {
    isHovered_ = false;
    if (isHoveringClear_) {
        isHoveringClear_ = false;
        setCursorShape(platform::CursorShape::IBeam);
    }
    updateBorderAnimation(true, hasFocus());
}

void TextInput::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (ev.button() == platform::MouseButton::Left) {
        setFocus(FocusReason::Pointer);
        ev.accept();

        if (shouldShowClearButton() && clearButtonLocalRect().contains(ev.position())) {
            isDraggingSelection_ = false;
            setText("");
            isHoveringClear_ = false;
            setCursorShape(platform::CursorShape::IBeam);
            return;
        }

        isDraggingSelection_ = true;

        float padL = effectiveTextLeftInset(layoutRect().size.width);
        size_t idx = hitTestCharacter(ev.position().x - padL);
        cursorIndex_ = idx;
        selectionAnchor_ = idx;
        ensureCursorVisible();
        markPaintDirty();
    }
}

void TextInput::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (isDraggingSelection_) {
        ev.accept();
        float padL = effectiveTextLeftInset(layoutRect().size.width);
        size_t idx = hitTestCharacter(ev.position().x - padL);
        cursorIndex_ = idx;
        ensureCursorVisible();
        markPaintDirty();
        return;
    }

    bool hoverClear = shouldShowClearButton() && clearButtonLocalRect().contains(ev.position());
    if (hoverClear != isHoveringClear_) {
        isHoveringClear_ = hoverClear;
        setCursorShape(hoverClear ? platform::CursorShape::PointingHand
                                  : platform::CursorShape::IBeam);
        markPaintDirty();
    }
}

void TextInput::onMouseUp(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left && isDraggingSelection_) {
        isDraggingSelection_ = false;
        ev.accept();
        markPaintDirty();
    }
}

void TextInput::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled() || !hasFocus()) {
        return;
    }
    cursorBlinkTimer_ = 0.0f;
    cursorVisible_ = true;
    markPaintDirty();
    bool shift = platform::hasModifier(ev.modifiers(), platform::KeyModifier::Shift);
    bool ctrl = platform::hasModifier(ev.modifiers(), platform::KeyModifier::Control);

    if (ctrl) {
        if (ev.key() == platform::Key::A) {
            selectAll();
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::C) {
            if (hasSelection() && !passwordMode_) {
                (void)platform::Clipboard::setText(selectedText());
            }
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::X) {
            if (hasSelection() && !passwordMode_) {
                (void)platform::Clipboard::setText(selectedText());
                if (!readOnly_) {
                    deleteSelection();
                }
            }
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::V) {
            if (!readOnly_) {
                auto clipRes = platform::Clipboard::getText();
                if (clipRes.isOk()) {
                    insertText(clipRes.value());
                }
            }
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::Left) {
            cursorIndex_ = prevWordBoundary(text_, cursorIndex_);
            if (!shift) {
                selectionAnchor_ = cursorIndex_;
            }
            ensureCursorVisible();
            markPaintDirty();
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::Right) {
            cursorIndex_ = nextWordBoundary(text_, cursorIndex_);
            if (!shift) {
                selectionAnchor_ = cursorIndex_;
            }
            ensureCursorVisible();
            markPaintDirty();
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::Backspace) {
            if (!readOnly_) {
                if (hasSelection()) {
                    deleteSelection();
                } else if (cursorIndex_ > 0) {
                    size_t prev = prevWordBoundary(text_, cursorIndex_);
                    text_.erase(prev, cursorIndex_ - prev);
                    cursorIndex_ = prev;
                    selectionAnchor_ = cursorIndex_;
                    invalidateTextCache();
                    ensureCursorVisible();
                    markPaintDirty();
                    onTextChanged.emit(text_);
                }
            }
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::Delete) {
            if (!readOnly_) {
                if (hasSelection()) {
                    deleteSelection();
                } else if (cursorIndex_ < text_.size()) {
                    size_t next = nextWordBoundary(text_, cursorIndex_);
                    text_.erase(cursorIndex_, next - cursorIndex_);
                    invalidateTextCache();
                    ensureCursorVisible();
                    markPaintDirty();
                    onTextChanged.emit(text_);
                }
            }
            ev.accept();
            return;
        }
    }

    if (ev.key() == platform::Key::Left) {
        if (cursorIndex_ > 0) {
            cursorIndex_ = prevUtf8Char(text_, cursorIndex_);
            if (!shift) {
                selectionAnchor_ = cursorIndex_;
            }
            ensureCursorVisible();
            markPaintDirty();
        } else if (!shift && hasSelection()) {
            cursorIndex_ = std::min(cursorIndex_, selectionAnchor_);
            selectionAnchor_ = cursorIndex_;
            ensureCursorVisible();
            markPaintDirty();
        }
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Right) {
        if (cursorIndex_ < text_.size()) {
            cursorIndex_ = nextUtf8Char(text_, cursorIndex_);
            if (!shift) {
                selectionAnchor_ = cursorIndex_;
            }
            ensureCursorVisible();
            markPaintDirty();
        } else if (!shift && hasSelection()) {
            cursorIndex_ = std::max(cursorIndex_, selectionAnchor_);
            selectionAnchor_ = cursorIndex_;
            ensureCursorVisible();
            markPaintDirty();
        }
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Home) {
        cursorIndex_ = 0;
        if (!shift) {
            selectionAnchor_ = cursorIndex_;
        }
        ensureCursorVisible();
        markPaintDirty();
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::End) {
        cursorIndex_ = text_.size();
        if (!shift) {
            selectionAnchor_ = cursorIndex_;
        }
        ensureCursorVisible();
        markPaintDirty();
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Backspace) {
        if (!readOnly_) {
            if (hasSelection()) {
                deleteSelection();
            } else if (cursorIndex_ > 0) {
                size_t prev = prevUtf8Char(text_, cursorIndex_);
                text_.erase(prev, cursorIndex_ - prev);
                cursorIndex_ = prev;
                selectionAnchor_ = cursorIndex_;
                invalidateTextCache();
                ensureCursorVisible();
                markPaintDirty();
                onTextChanged.emit(text_);
            }
        }
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Delete) {
        if (!readOnly_) {
            if (hasSelection()) {
                deleteSelection();
            } else if (cursorIndex_ < text_.size()) {
                size_t next = nextUtf8Char(text_, cursorIndex_);
                text_.erase(cursorIndex_, next - cursorIndex_);
                invalidateTextCache();
                ensureCursorVisible();
                markPaintDirty();
                onTextChanged.emit(text_);
            }
        }
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Enter) {
        onEnterPressed.emit(text_);
        ev.accept();
        return;
    }

    uint32_t cp = ev.textCodepoint();
    if (cp >= 32 && cp != 127 && !ctrl && !readOnly_) {
        std::string utf8 = codepointToUtf8(cp);
        insertText(utf8);
        ev.accept();
    }
}

void TextInput::onFocusGained() {
    cursorBlinkTimer_ = 0.0f;
    cursorVisible_ = true;
    updateBorderAnimation(true, true);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void TextInput::onFocusLost() {
    isDraggingSelection_ = false;
    selectionAnchor_ = cursorIndex_;
    cursorVisible_ = false;
    updateBorderAnimation(true, false);
    markPaintDirty();
}

void TextInput::updateAnimations(float deltaSeconds) {
    bool dirty = false;
    if (hoverAnim_.isRunning()) {
        hoverAnim_.update(deltaSeconds);
        dirty = true;
    }
    if (focusAnim_.isRunning()) {
        focusAnim_.update(deltaSeconds);
        dirty = true;
    }
    if (hasFocus()) {
        cursorBlinkTimer_ += deltaSeconds;
        bool visible = (std::fmod(cursorBlinkTimer_, 1.06f) < 0.53f);
        if (visible != cursorVisible_) {
            cursorVisible_ = visible;
            dirty = true;
        }
    }
    if (dirty) {
        markPaintDirty();
    }
}

bool TextInput::hasActiveAnimations() const noexcept {
    return (hasFocus() && isEnabled()) || hoverAnim_.isRunning() || focusAnim_.isRunning();
}

void TextInput::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    const auto& pal = colorPalette();
    float hoverT = hoverAnim_.isRunning() ? hoverAnim_.currentValue() : (isHovered_ ? 1.0f : 0.0f);
    float focusT = focusAnim_.isRunning() ? focusAnim_.currentValue() : (hasFocus() ? 1.0f : 0.0f);

    core::Color borderCol = pal.borderSubtle;
    if (isEnabled()) {
        core::Color idleCol = overrideIdleBorderColor_.value_or(pal.borderDefault);
        core::Color hoverCol = pal.borderStrong;
        core::Color focusCol = overrideFocusedBorderColor_.value_or(pal.accentDefault);
        borderCol = idleCol.lerp(hoverCol, hoverT).lerp(focusCol, focusT);
    }

    ctx.drawBox(rect, effectiveBackgroundColor(), borderCol, borderWidth_, cornerRadii_);

    if (isEnabled() && focusT > 0.001f) {
        core::Color ringCol = pal.accentFocusRing.withAlpha(pal.accentFocusRing.a * focusT);
        theme::paintFocusRing(ctx, rect, cornerRadii_[0], theme::Metrics::FocusRingOffset,
                              theme::Metrics::FocusRingWidth, ringCol);
    }
    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;

    if (leadingIcon_ && leadingIcon_->isValid()) {
        float padL = style().padding.left.resolve(rect.size.width, 0.0f);
        float iconSize = 14.0f * dpiScale_;
        core::Rect iconRect{
            core::Point{rect.origin.x + padL, std::round(targetCenterY - iconSize * 0.5f)},
            core::Size{iconSize, iconSize}};
        core::Color iconCol = isEnabled() ? pal.textTertiary : pal.textDisabled;
        ctx.drawSvg(*leadingIcon_, iconRect, iconCol);
    }

    float leftInset = effectiveTextLeftInset(rect.size.width);
    float rightInset = effectiveTextRightInset(rect.size.width);

    core::Rect textClipRect{
        core::Point{rect.origin.x + leftInset, rect.origin.y + 1.0f},
        core::Size{std::max(0.0f, rect.size.width - leftInset - rightInset),
                   std::max(0.0f, rect.size.height - 2.0f)},
    };

    if (textClipRect.size.width > 0.0f && textClipRect.size.height > 0.0f) {
        ctx.pushClip(textClipRect);

        auto f = effectiveFont();
        float textX = textClipRect.origin.x - scrollOffset_;
        float textY = f ? theme::computeCenteredTextY(targetCenterY, *f) : targetCenterY;
        float cursorH = f ? std::min(textClipRect.size.height, f->pointSize() * 1.3f) : 16.0f;
        float cursorY = targetCenterY - cursorH * 0.5f;

        if (text_.empty() && !placeholder_.empty() && f) {
            ensurePlaceholderCache();
            if (cachedPlaceholderLayout_) {
                ctx.drawTextLayout(core::Point{textX, textY}, *cachedPlaceholderLayout_,
                                   effectivePlaceholderColor());
            }
        } else if (!text_.empty() && f) {
            ensureTextCache();
            if (hasSelection()) {
                size_t selStart = std::min(cursorIndex_, selectionAnchor_);
                size_t selEnd = std::max(cursorIndex_, selectionAnchor_);
                float selStartX = measureOffsetAt(selStart);
                float selEndX = measureOffsetAt(selEnd);
                core::Rect selRect{
                    core::Point{textX + selStartX, cursorY},
                    core::Size{selEndX - selStartX, cursorH},
                };
                ctx.drawRoundedRect(selRect, effectiveSelectionColor(), 3.0f * dpiScale_);
            }

            if (cachedTextLayout_) {
                ctx.drawTextLayout(core::Point{textX, textY}, *cachedTextLayout_,
                                   effectiveTextColor());
            }
        }

        if (hasFocus() && isEnabled() && !readOnly_ && f && cursorVisible_) {
            float cursorX = measureOffsetAt(cursorIndex_);
            core::Rect cursorRect{core::Point{textX + cursorX, cursorY},
                                  core::Size{1.5f * dpiScale_, cursorH}};
            ctx.drawRect(cursorRect, effectiveCursorColor());
        }

        ctx.popClip();
    }

    if (shouldShowClearButton()) {
        core::Rect localClear = clearButtonLocalRect();
        core::Rect screenClear{
            core::Point{rect.origin.x + localClear.origin.x, rect.origin.y + localClear.origin.y},
            localClear.size};
        float r = screenClear.size.width * 0.5f;
        core::Color pillBg = isHoveringClear_ ? pal.surfaceOverlay : pal.surfaceRaised;
        ctx.drawRoundedRect(screenClear, pillBg, r);

        if (auto closeDoc = getSvgDocument(IconId::Close)) {
            float iconSize = 10.0f * dpiScale_;
            core::Rect iconRect{
                core::Point{screenClear.origin.x + (screenClear.size.width - iconSize) * 0.5f,
                            screenClear.origin.y + (screenClear.size.height - iconSize) * 0.5f},
                core::Size{iconSize, iconSize}};
            ctx.drawSvg(*closeDoc, iconRect,
                        isHoveringClear_ ? pal.textPrimary : pal.textSecondary);
        }
    }
}

} // namespace northern::widgets
