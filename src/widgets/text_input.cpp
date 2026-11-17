#include "widgets/text_input.hpp"
#include "platform/clipboard.hpp"
#include "renderer/paint_context.hpp"
#include "text/text_layout.hpp"
#include "widgets/theme.hpp"

#include <algorithm>

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

size_t prevUtf8Char(const std::string& str, size_t idx) {
    if (idx == 0 || idx > str.size()) {
        return 0;
    }
    --idx;
    while (idx > 0 && (static_cast<unsigned char>(str[idx]) & 0xC0) == 0x80) {
        --idx;
    }
    return idx;
}

size_t nextUtf8Char(const std::string& str, size_t idx) {
    if (idx >= str.size()) {
        return str.size();
    }
    ++idx;
    while (idx < str.size() && (static_cast<unsigned char>(str[idx]) & 0xC0) == 0x80) {
        ++idx;
    }
    return idx;
}

float measurePrefixWidth(std::string_view text, size_t count,
                         const std::shared_ptr<text::Font>& font) {
    if (count == 0 || !font || text.empty()) {
        return 0.0f;
    }
    size_t len = std::min(count, text.size());
    auto layout = text::TextLayout::create(text.substr(0, len), font);
    return layout.size().width;
}

} // namespace

TextInput::TextInput(std::string placeholder, std::shared_ptr<text::Font> font)
    : placeholder_(std::move(placeholder)), font_(std::move(font)) {
    setFocusable(true);
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
        float textH = 20.0f;
        if (font_) {
            std::string_view sample = text_.empty() ? placeholder_ : text_;
            if (!sample.empty()) {
                auto temp = text::TextLayout::create(sample, font_);
                textW = temp.size().width;
                textH = std::max(textH, temp.size().height);
            }
        }
        float padH = style().padding.left.resolve(availW, 0.0f) +
                     style().padding.right.resolve(availW, 0.0f);
        float padV =
            style().padding.top.resolve(0.0f, 0.0f) + style().padding.bottom.resolve(0.0f, 0.0f);
        return core::Size{std::max(120.0f, textW + padH), textH + padV};
    });
}

void TextInput::setText(std::string text) {
    if (text_ != text) {
        text_ = std::move(text);
        cursorIndex_ = std::min(cursorIndex_, text_.size());
        selectionAnchor_ = cursorIndex_;
        ensureCursorVisible();
        markPaintDirty();
        onTextChanged.emit(text_);
    }
}

void TextInput::setPlaceholder(std::string placeholder) {
    if (placeholder_ != placeholder) {
        placeholder_ = std::move(placeholder);
        markPaintDirty();
    }
}

void TextInput::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        markLayoutDirty();
    }
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
    bgColor_ = color;
    markPaintDirty();
}

void TextInput::setIdleBorderColor(const core::Color& color) noexcept {
    idleBorderColor_ = color;
    markPaintDirty();
}

void TextInput::setFocusedBorderColor(const core::Color& color) noexcept {
    focusedBorderColor_ = color;
    markPaintDirty();
}

void TextInput::setTextColor(const core::Color& color) noexcept {
    textColor_ = color;
    markPaintDirty();
}

void TextInput::setPlaceholderColor(const core::Color& color) noexcept {
    placeholderColor_ = color;
    markPaintDirty();
}

void TextInput::setSelectionColor(const core::Color& color) noexcept {
    selectionColor_ = color;
    markPaintDirty();
}

void TextInput::setCursorColor(const core::Color& color) noexcept {
    cursorColor_ = color;
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

void TextInput::deleteSelection() {
    if (!hasSelection()) {
        return;
    }
    size_t start = std::min(cursorIndex_, selectionAnchor_);
    size_t end = std::max(cursorIndex_, selectionAnchor_);
    text_.erase(start, end - start);
    cursorIndex_ = start;
    selectionAnchor_ = cursorIndex_;
    ensureCursorVisible();
    markPaintDirty();
    onTextChanged.emit(text_);
}

void TextInput::insertText(std::string_view str) {
    if (str.empty()) {
        return;
    }
    if (hasSelection()) {
        deleteSelection();
    }
    text_.insert(cursorIndex_, str);
    cursorIndex_ += str.size();
    selectionAnchor_ = cursorIndex_;
    ensureCursorVisible();
    markPaintDirty();
    onTextChanged.emit(text_);
}

size_t TextInput::hitTestCharacter(float localX) const {
    if (text_.empty() || !font_) {
        return 0;
    }
    float adjustedX = localX + scrollOffset_;
    if (adjustedX <= 0.0f) {
        return 0;
    }

    size_t bestIdx = 0;
    float prevW = 0.0f;
    size_t i = 0;
    while (i <= text_.size()) {
        float w = measurePrefixWidth(text_, i, font_);
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
    if (!font_) {
        return;
    }
    float cursorX = measurePrefixWidth(text_, cursorIndex_, font_);
    float contentW = layoutNode_.contentRect().size.width;
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

void TextInput::onMouseDown(WidgetMouseEvent& ev) {
    if (ev.button() == platform::MouseButton::Left) {
        setFocus();
        ev.accept();
        isDraggingSelection_ = true;

        float padL = style().padding.left.resolve(layoutRect().size.width, 0.0f);
        size_t idx = hitTestCharacter(ev.position().x - padL);
        cursorIndex_ = idx;
        selectionAnchor_ = idx;
        ensureCursorVisible();
        markPaintDirty();
    }
}

void TextInput::onMouseMove(WidgetMouseEvent& ev) {
    if (isDraggingSelection_) {
        ev.accept();
        float padL = style().padding.left.resolve(layoutRect().size.width, 0.0f);
        size_t idx = hitTestCharacter(ev.position().x - padL);
        cursorIndex_ = idx;
        ensureCursorVisible();
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
    if (!hasFocus()) {
        return;
    }
    bool shift = platform::hasModifier(ev.modifiers(), platform::KeyModifier::Shift);
    bool ctrl = platform::hasModifier(ev.modifiers(), platform::KeyModifier::Control);

    if (ctrl) {
        if (ev.key() == platform::Key::A) {
            selectAll();
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::C) {
            if (hasSelection()) {
                (void)platform::Clipboard::setText(selectedText());
            }
            ev.accept();
            return;
        }
        if (ev.key() == platform::Key::V) {
            auto clipRes = platform::Clipboard::getText();
            if (clipRes.isOk()) {
                insertText(clipRes.value());
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
        if (hasSelection()) {
            deleteSelection();
        } else if (cursorIndex_ > 0) {
            size_t prev = prevUtf8Char(text_, cursorIndex_);
            text_.erase(prev, cursorIndex_ - prev);
            cursorIndex_ = prev;
            selectionAnchor_ = cursorIndex_;
            ensureCursorVisible();
            markPaintDirty();
            onTextChanged.emit(text_);
        }
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Delete) {
        if (hasSelection()) {
            deleteSelection();
        } else if (cursorIndex_ < text_.size()) {
            size_t next = nextUtf8Char(text_, cursorIndex_);
            text_.erase(cursorIndex_, next - cursorIndex_);
            ensureCursorVisible();
            markPaintDirty();
            onTextChanged.emit(text_);
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
    if (cp >= 32 && cp != 127 && !ctrl) {
        std::string utf8 = codepointToUtf8(cp);
        insertText(utf8);
        ev.accept();
    }
}

void TextInput::onFocusGained() {
    markPaintDirty();
}

void TextInput::onFocusLost() {
    isDraggingSelection_ = false;
    selectionAnchor_ = cursorIndex_;
    markPaintDirty();
}

void TextInput::paint(renderer::PaintContext& ctx) {
    core::Rect rect = screenRect();
    if (rect.size.width <= 0.0f || rect.size.height <= 0.0f) {
        return;
    }

    const core::Color& border = hasFocus() ? focusedBorderColor_ : idleBorderColor_;
    ctx.drawBox(rect, bgColor_, border, borderWidth_, cornerRadii_);

    if (hasFocus()) {
        theme::paintFocusRing(ctx, rect, cornerRadii_[0]);
    }

    float padL = style().padding.left.resolve(rect.size.width, 0.0f);
    float padR = style().padding.right.resolve(rect.size.width, 0.0f);

    core::Rect textClipRect{
        core::Point{rect.origin.x + padL, rect.origin.y + 1.0f},
        core::Size{std::max(0.0f, rect.size.width - padL - padR),
                   std::max(0.0f, rect.size.height - 2.0f)},
    };

    if (textClipRect.size.width <= 0.0f || textClipRect.size.height <= 0.0f) {
        return;
    }

    ctx.pushClip(textClipRect);

    float targetCenterY = rect.origin.y + rect.size.height * 0.5f;
    float textX = textClipRect.origin.x - scrollOffset_;
    float textY = font_ ? theme::computeCenteredTextY(targetCenterY, *font_) : targetCenterY;
    float cursorH = font_ ? std::min(textClipRect.size.height, font_->pointSize() * 1.3f) : 16.0f;
    float cursorY = targetCenterY - cursorH * 0.5f;

    if (text_.empty() && !placeholder_.empty() && !hasFocus()) {
        if (font_) {
            auto placeLayout = text::TextLayout::create(placeholder_, font_);
            ctx.drawTextLayout(core::Point{textX, textY}, placeLayout, placeholderColor_);
        }
    } else if (!text_.empty() && font_) {
        if (hasSelection()) {
            size_t selStart = std::min(cursorIndex_, selectionAnchor_);
            size_t selEnd = std::max(cursorIndex_, selectionAnchor_);
            float selStartX = measurePrefixWidth(text_, selStart, font_);
            float selEndX = measurePrefixWidth(text_, selEnd, font_);
            core::Rect selRect{
                core::Point{textX + selStartX, cursorY},
                core::Size{selEndX - selStartX, cursorH},
            };
            ctx.drawRect(selRect, selectionColor_);
        }

        auto textLayout = text::TextLayout::create(text_, font_);
        ctx.drawTextLayout(core::Point{textX, textY}, textLayout, textColor_);
    }

    if (hasFocus() && font_) {
        float cursorX = measurePrefixWidth(text_, cursorIndex_, font_);
        core::Rect cursorRect{core::Point{textX + cursorX, cursorY}, core::Size{1.5f, cursorH}};
        ctx.drawRect(cursorRect, cursorColor_);
    }

    ctx.popClip();
}

} // namespace northern::widgets
