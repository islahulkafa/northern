#include "widgets/controls/text_area.hpp"

#include "platform/clipboard.hpp"
#include "renderer/paint_context.hpp"
#include "text/font_manager.hpp"
#include "text/text_shaper.hpp"
#include "widgets/base/widget_context.hpp"

#include <algorithm>
#include <cmath>

namespace northern::widgets {

namespace {

constexpr float kDefaultTextPt = 14.0f;
constexpr float kPaddingX = 10.0f;
constexpr float kPaddingY = 8.0f;
constexpr float kMinHeight = 84.0f;
constexpr float kMinWidth = 120.0f;
constexpr size_t kMaxUndoHistory = 128;
constexpr float kUndoCoalesceMs = 600.0f;
constexpr float kMultiClickMaxMs = 400.0f;

std::string normalizeNewlines(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\r') {
            out.push_back('\n');
            if (i + 1 < input.size() && input[i + 1] == '\n') {
                ++i;
            }
        } else {
            out.push_back(input[i]);
        }
    }
    return out;
}

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

bool isWhitespaceChar(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool isWordSeparator(char c) noexcept {
    return isWhitespaceChar(c) || c == '.' || c == ',' || c == ';' || c == ':' || c == '(' ||
           c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == '/' || c == '\\' ||
           c == '-' || c == '=' || c == '+' || c == '"';
}

size_t prevWordBoundary(std::string_view str, size_t idx) {
    if (idx == 0 || idx > str.size()) {
        return 0;
    }
    while (idx > 0) {
        size_t prev = prevUtf8Char(str, idx);
        if (!isWhitespaceChar(str[prev])) {
            break;
        }
        idx = prev;
    }
    while (idx > 0) {
        size_t prev = prevUtf8Char(str, idx);
        if (isWhitespaceChar(str[prev])) {
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
    while (idx < str.size() && !isWhitespaceChar(str[idx])) {
        idx = nextUtf8Char(str, idx);
    }
    while (idx < str.size() && isWhitespaceChar(str[idx])) {
        idx = nextUtf8Char(str, idx);
    }
    return idx;
}

std::pair<size_t, size_t> wordBoundsAt(std::string_view str, size_t idx) {
    if (str.empty()) {
        return {0, 0};
    }
    if (idx >= str.size()) {
        idx = prevUtf8Char(str, str.size());
    }
    if (isWordSeparator(str[idx])) {
        return {idx, nextUtf8Char(str, idx)};
    }
    size_t start = idx;
    while (start > 0) {
        size_t prev = prevUtf8Char(str, start);
        if (isWordSeparator(str[prev])) {
            break;
        }
        start = prev;
    }
    size_t end = nextUtf8Char(str, idx);
    while (end < str.size() && !isWordSeparator(str[end])) {
        end = nextUtf8Char(str, end);
    }
    return {start, end};
}

std::pair<size_t, size_t> paragraphBoundsAt(std::string_view str, size_t idx) {
    size_t start = idx;
    while (start > 0 && str[start - 1] != '\n') {
        --start;
    }
    size_t end = idx;
    while (end < str.size() && str[end] != '\n') {
        ++end;
    }
    return {start, end};
}

} // namespace

TextArea::TextArea(std::string placeholder, std::shared_ptr<text::Font> font)
    : placeholder_(std::move(placeholder)), font_(std::move(font)) {
    setFocusable(true);
    setCursorShape(platform::CursorShape::IBeam);
    style().minWidth = layout::Length::points(kMinWidth);
    style().minHeight = layout::Length::points(kMinHeight);

    layoutNode_.setMeasureFunc(
        [this](const layout::LayoutNode&, float availW, float) -> core::Size {
            float w = availW > 0.0f ? availW : 260.0f * dpiScale_;
            float h = std::max(kMinHeight * dpiScale_,
                               4.0f * effectiveLineHeight() + 2.0f * kPaddingY * dpiScale_);
            return core::Size{w, h};
        });
}

void TextArea::setText(std::string text) {
    std::string normalized = normalizeNewlines(text);
    if (maxLength_ > 0 && utf8CodepointCount(normalized) > maxLength_) {
        normalized = std::string(truncateToCodepoints(normalized, maxLength_));
    }
    if (text_ != normalized) {
        text_ = std::move(normalized);
        cursorIndex_ = text_.size();
        selectionAnchor_ = text_.size();
        desiredCaretX_ = -1.0f;
        clearUndoHistory();
        invalidateLineCache();
        clampScrollOffsets();
        markPaintDirty();
        onTextChanged.emit(text_);
        onSelectionChanged.emit();
        onCursorPositionChanged.emit(cursorLine(), cursorColumn());
    }
}

void TextArea::setPlaceholder(std::string placeholder) {
    if (placeholder_ != placeholder) {
        placeholder_ = std::move(placeholder);
        placeholderCacheValid_ = false;
        markPaintDirty();
    }
}

void TextArea::setFont(std::shared_ptr<text::Font> font) {
    if (font_ != font) {
        font_ = std::move(font);
        resolvedDefaultFont_.reset();
        invalidateLineCache();
        placeholderCacheValid_ = false;
        markLayoutDirty();
        markPaintDirty();
    }
}

std::shared_ptr<text::Font> TextArea::effectiveFont() const {
    if (font_) {
        return font_;
    }
    if (!resolvedDefaultFont_) {
        float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
        resolvedDefaultFont_ =
            text::FontManager::instance().defaultFont(kDefaultTextPt * scale).valueOr(nullptr);
    }
    return resolvedDefaultFont_;
}

void TextArea::setDpiScale(float scale) {
    hasCustomDpiScale_ = true;
    applyDpiScale(scale);
}

void TextArea::applyDpiScale(float scale) {
    float clamped = scale > 0.0f ? scale : 1.0f;
    if (dpiScale_ != clamped) {
        dpiScale_ = clamped;
        scrollbarThickness_ = 6.0f * dpiScale_;
        style().minWidth = layout::Length::points(kMinWidth * dpiScale_);
        style().minHeight = layout::Length::points(kMinHeight * dpiScale_);
        resolvedDefaultFont_.reset();
        invalidateLineCache();
        placeholderCacheValid_ = false;
        clampScrollOffsets();
        markLayoutDirty();
        markPaintDirty();
    }
}

void TextArea::setReadOnly(bool readOnly) noexcept {
    if (readOnly_ != readOnly) {
        readOnly_ = readOnly;
        markPaintDirty();
    }
}

void TextArea::setMaxLength(size_t maxChars) {
    maxLength_ = maxChars;
    if (maxLength_ > 0 && utf8CodepointCount(text_) > maxLength_) {
        std::string truncated(truncateToCodepoints(text_, maxLength_));
        text_ = std::move(truncated);
        cursorIndex_ = std::min(cursorIndex_, text_.size());
        selectionAnchor_ = std::min(selectionAnchor_, text_.size());
        invalidateLineCache();
        clampScrollOffsets();
        markPaintDirty();
        onTextChanged.emit(text_);
    }
}

void TextArea::setTabWidth(int spaces) noexcept {
    tabWidth_ = std::clamp(spaces, 1, 16);
}

void TextArea::setWordWrap(text::WordWrap wrap) {
    if (wordWrap_ != wrap) {
        wordWrap_ = wrap;
        if (wordWrap_ == text::WordWrap::Word) {
            scrollOffsetX_ = 0.0f;
        }
        invalidateLineCache();
        clampScrollOffsets();
        ensureCursorVisible();
        markPaintDirty();
    }
}

void TextArea::setLineHeightMultiplier(float multiplier) {
    float clamped = std::clamp(multiplier, 1.0f, 3.0f);
    if (lineHeightMultiplier_ != clamped) {
        lineHeightMultiplier_ = clamped;
        invalidateLineCache();
        clampScrollOffsets();
        markLayoutDirty();
        markPaintDirty();
    }
}

void TextArea::setShowLineNumbers(bool show) {
    if (showLineNumbers_ != show) {
        showLineNumbers_ = show;
        invalidateLineCache();
        clampScrollOffsets();
        markPaintDirty();
    }
}

std::string TextArea::selectedText() const {
    if (!hasSelection()) {
        return {};
    }
    size_t start = std::min(cursorIndex_, selectionAnchor_);
    size_t end = std::max(cursorIndex_, selectionAnchor_);
    return text_.substr(start, end - start);
}

int TextArea::cursorLine() const noexcept {
    int line = 0;
    size_t limit = std::min(cursorIndex_, text_.size());
    for (size_t i = 0; i < limit; ++i) {
        if (text_[i] == '\n') {
            ++line;
        }
    }
    return line;
}

int TextArea::cursorColumn() const noexcept {
    size_t lineStart = std::min(cursorIndex_, text_.size());
    while (lineStart > 0 && text_[lineStart - 1] != '\n') {
        --lineStart;
    }
    return static_cast<int>(utf8CodepointCount(std::string_view(text_).substr(
        lineStart, std::min(cursorIndex_, text_.size()) - lineStart)));
}

int TextArea::lineCount() const noexcept {
    ensureLineCache();
    return logicalLineCount_;
}

void TextArea::selectAll() {
    size_t prevC = cursorIndex_;
    size_t prevA = selectionAnchor_;
    selectionAnchor_ = 0;
    cursorIndex_ = text_.size();
    desiredCaretX_ = -1.0f;
    notifyCursorAndSelection(prevC, prevA);
    markPaintDirty();
}

void TextArea::setCursorIndex(size_t index, bool keepSelection) {
    size_t prevC = cursorIndex_;
    size_t prevA = selectionAnchor_;
    size_t clamped = std::min(index, text_.size());
    cursorIndex_ = clamped;
    if (!keepSelection) {
        selectionAnchor_ = clamped;
    }
    desiredCaretX_ = -1.0f;
    resetBlink();
    ensureCursorVisible();
    notifyCursorAndSelection(prevC, prevA);
    markPaintDirty();
}

void TextArea::clearUndoHistory() noexcept {
    undoStack_.clear();
    redoStack_.clear();
    lastEditKind_ = EditKind::None;
}

void TextArea::pushUndoSnapshot(EditKind kind) {
    auto now = std::chrono::steady_clock::now();
    bool canCoalesce = false;
    if (kind != EditKind::None && kind == lastEditKind_ && !undoStack_.empty()) {
        float elapsedMs = std::chrono::duration<float, std::milli>(now - lastEditTime_).count();
        if (elapsedMs <= kUndoCoalesceMs) {
            canCoalesce = true;
        }
    }
    if (!canCoalesce) {
        if (undoStack_.size() >= kMaxUndoHistory) {
            undoStack_.erase(undoStack_.begin());
        }
        undoStack_.push_back(HistoryEntry{text_, cursorIndex_, selectionAnchor_});
    }
    redoStack_.clear();
    lastEditKind_ = kind;
    lastEditTime_ = now;
}

void TextArea::applyEdit(std::string newText, size_t newCursor, size_t newAnchor, EditKind kind) {
    pushUndoSnapshot(kind);
    size_t prevC = cursorIndex_;
    size_t prevA = selectionAnchor_;
    text_ = std::move(newText);
    cursorIndex_ = std::min(newCursor, text_.size());
    selectionAnchor_ = std::min(newAnchor, text_.size());
    desiredCaretX_ = -1.0f;
    resetBlink();
    invalidateLineCache();
    ensureCursorVisible();
    markPaintDirty();
    onTextChanged.emit(text_);
    notifyCursorAndSelection(prevC, prevA);
}

void TextArea::undo() {
    if (readOnly_ || undoStack_.empty()) {
        return;
    }
    size_t prevC = cursorIndex_;
    size_t prevA = selectionAnchor_;
    redoStack_.push_back(HistoryEntry{text_, cursorIndex_, selectionAnchor_});
    HistoryEntry entry = std::move(undoStack_.back());
    undoStack_.pop_back();
    lastEditKind_ = EditKind::None;

    text_ = std::move(entry.text);
    cursorIndex_ = std::min(entry.cursorIndex, text_.size());
    selectionAnchor_ = std::min(entry.selectionAnchor, text_.size());
    desiredCaretX_ = -1.0f;
    resetBlink();
    invalidateLineCache();
    ensureCursorVisible();
    markPaintDirty();
    onTextChanged.emit(text_);
    notifyCursorAndSelection(prevC, prevA);
}

void TextArea::redo() {
    if (readOnly_ || redoStack_.empty()) {
        return;
    }
    size_t prevC = cursorIndex_;
    size_t prevA = selectionAnchor_;
    undoStack_.push_back(HistoryEntry{text_, cursorIndex_, selectionAnchor_});
    HistoryEntry entry = std::move(redoStack_.back());
    redoStack_.pop_back();
    lastEditKind_ = EditKind::None;

    text_ = std::move(entry.text);
    cursorIndex_ = std::min(entry.cursorIndex, text_.size());
    selectionAnchor_ = std::min(entry.selectionAnchor, text_.size());
    desiredCaretX_ = -1.0f;
    resetBlink();
    invalidateLineCache();
    ensureCursorVisible();
    markPaintDirty();
    onTextChanged.emit(text_);
    notifyCursorAndSelection(prevC, prevA);
}

void TextArea::notifyCursorAndSelection(size_t prevCursor, size_t prevAnchor) {
    if (cursorIndex_ != prevCursor) {
        onCursorPositionChanged.emit(cursorLine(), cursorColumn());
    }
    if (cursorIndex_ != prevCursor || selectionAnchor_ != prevAnchor) {
        onSelectionChanged.emit();
    }
}

void TextArea::resetBlink() noexcept {
    cursorBlinkTimer_ = 0.0f;
    cursorVisible_ = true;
}

float TextArea::effectiveLineHeight() const {
    auto f = effectiveFont();
    float base = f ? f->lineHeight() : (14.0f * dpiScale_);
    return std::max(8.0f * dpiScale_, base * lineHeightMultiplier_);
}

float TextArea::gutterWidth() const {
    if (!showLineNumbers_) {
        return 0.0f;
    }
    int digits = 2;
    int lines = std::max(1, logicalLineCount_);
    int d = 0;
    while (lines > 0) {
        lines /= 10;
        ++d;
    }
    digits = std::max(2, d);
    return static_cast<float>(digits) * 8.0f * dpiScale_ + 16.0f * dpiScale_;
}

core::Rect TextArea::textViewportLocalRect() const {
    core::Size sz = layoutRect().size;
    float gw = gutterWidth();
    float padX = kPaddingX * dpiScale_;
    float padY = kPaddingY * dpiScale_;
    float x = gw + padX;
    float w =
        std::max(0.0f, sz.width - gw - padX * 2.0f - (scrollbarThickness_ + 4.0f * dpiScale_));
    float h = std::max(0.0f, sz.height - padY * 2.0f);
    return core::Rect{core::Point{x, padY}, core::Size{w, h}};
}

void TextArea::invalidateLineCache() noexcept {
    lineCacheValid_ = false;
    visualLines_.clear();
}

void TextArea::ensurePlaceholderCache() const {
    if (placeholderCacheValid_) {
        return;
    }
    cachedPlaceholderLayout_.reset();
    auto f = effectiveFont();
    if (f && !placeholder_.empty()) {
        text::TextLayoutOptions opts{};
        opts.maxWidth = std::max(0.0f, textViewportLocalRect().size.width);
        opts.wrapMode = text::WordWrap::Word;
        opts.overflow = text::TextOverflow::Clip;
        opts.lineHeightMultiplier = lineHeightMultiplier_;
        cachedPlaceholderLayout_ = text::TextLayout::create(placeholder_, f, opts);
    }
    placeholderCacheValid_ = true;
}

void TextArea::ensureLineCache() const {
    float wrapW = textViewportLocalRect().size.width;
    if (lineCacheValid_ &&
        (wordWrap_ == text::WordWrap::None || std::abs(cachedWrapWidth_ - wrapW) <= 0.5f)) {
        return;
    }

    visualLines_.clear();
    cachedWrapWidth_ = wrapW;
    contentMaxWidth_ = 0.0f;
    contentHeight_ = 0.0f;

    auto f = effectiveFont();
    float lh = effectiveLineHeight();
    float curY = 0.0f;
    int paraIndex = 0;

    auto buildClusterPenX = [&](std::string_view slice, const std::shared_ptr<text::Font>& font,
                                std::vector<float>& outPenX) {
        outPenX.assign(slice.size() + 1, 0.0f);
        if (!font || slice.empty()) {
            return;
        }
        auto glyphs = text::TextShaper::shape(*font, slice);
        float pen = 0.0f;
        size_t gIdx = 0;
        for (size_t b = 0; b <= slice.size(); ++b) {
            while (gIdx < glyphs.size() && glyphs[gIdx].cluster < b) {
                pen += glyphs[gIdx].advance.x;
                ++gIdx;
            }
            outPenX[b] = pen;
        }
    };

    auto pushVisualSlice = [&](size_t startB, size_t endB, int logicalLine, bool isFirst) {
        VisualLine vl;
        vl.startByte = startB;
        vl.endByte = endB;
        vl.logicalLine = logicalLine;
        vl.isFirstInParagraph = isFirst;
        vl.y = curY;

        std::string_view slice(text_.data() + startB, endB - startB);
        if (f && !slice.empty()) {
            text::TextLayoutOptions opts{};
            opts.maxWidth = 0.0f;
            opts.wrapMode = text::WordWrap::None;
            opts.overflow = text::TextOverflow::Clip;
            opts.lineHeightMultiplier = lineHeightMultiplier_;
            vl.layout = text::TextLayout::create(slice, f, opts);
            vl.width = vl.layout->size().width;
            buildClusterPenX(slice, f, vl.clusterPenX);
        } else {
            vl.width = 0.0f;
            vl.clusterPenX.assign(slice.size() + 1, 0.0f);
        }

        contentMaxWidth_ = std::max(contentMaxWidth_, vl.width);
        curY += lh;
        visualLines_.push_back(std::move(vl));
    };

    size_t start = 0;
    while (start <= text_.size()) {
        size_t nl = text_.find('\n', start);
        size_t end = (nl == std::string::npos) ? text_.size() : nl;
        std::string_view para(text_.data() + start, end - start);

        if (para.empty() || !f || wordWrap_ == text::WordWrap::None || wrapW <= 4.0f) {
            pushVisualSlice(start, end, paraIndex, true);
        } else {
            auto glyphs = text::TextShaper::shape(*f, para);
            if (glyphs.empty()) {
                pushVisualSlice(start, end, paraIndex, true);
            } else {
                std::vector<float> penX(glyphs.size() + 1, 0.0f);
                for (size_t i = 0; i < glyphs.size(); ++i) {
                    penX[i + 1] = penX[i] + glyphs[i].advance.x;
                }

                size_t cursor = 0;
                bool firstLine = true;
                while (cursor < glyphs.size()) {
                    float basePen = penX[cursor];
                    size_t fitEnd = cursor;
                    size_t lastSpace = std::string::npos;

                    for (size_t i = cursor; i < glyphs.size(); ++i) {
                        uint32_t cl = glyphs[i].cluster;
                        bool isSpace = cl < para.size() && (para[cl] == ' ' || para[cl] == '\t');
                        float widthWithGlyph = penX[i + 1] - basePen;
                        if (widthWithGlyph <= wrapW || i == cursor) {
                            fitEnd = i + 1;
                            if (isSpace) {
                                lastSpace = i + 1;
                            }
                        } else {
                            break;
                        }
                    }

                    size_t sliceEnd = fitEnd;
                    if (fitEnd < glyphs.size() && lastSpace != std::string::npos &&
                        lastSpace > cursor) {
                        sliceEnd = lastSpace;
                    }

                    size_t bStart = start + glyphs[cursor].cluster;
                    size_t bEnd =
                        (sliceEnd < glyphs.size()) ? (start + glyphs[sliceEnd].cluster) : end;
                    pushVisualSlice(bStart, bEnd, paraIndex, firstLine);
                    firstLine = false;
                    cursor = sliceEnd;
                }
            }
        }

        ++paraIndex;
        if (nl == std::string::npos) {
            break;
        }
        start = nl + 1;
    }

    logicalLineCount_ = std::max(1, paraIndex);
    contentHeight_ = curY;
    lineCacheValid_ = true;
}

size_t TextArea::visualLineForByte(size_t byteIndex) const {
    ensureLineCache();
    if (visualLines_.empty()) {
        return 0;
    }
    for (size_t i = 0; i < visualLines_.size(); ++i) {
        const auto& vl = visualLines_[i];
        bool isLastOnPara =
            (i + 1 >= visualLines_.size()) || (visualLines_[i + 1].logicalLine != vl.logicalLine);
        if (byteIndex >= vl.startByte &&
            (byteIndex < vl.endByte || (isLastOnPara && byteIndex <= vl.endByte))) {
            return i;
        }
    }
    return visualLines_.size() - 1;
}

float TextArea::xOffsetInVisualLine(const VisualLine& vl, size_t byteIndex) const {
    if (byteIndex <= vl.startByte || vl.clusterPenX.empty()) {
        return 0.0f;
    }
    size_t rel = std::min(byteIndex - vl.startByte, vl.clusterPenX.size() - 1);
    return vl.clusterPenX[rel];
}

size_t TextArea::hitTestPosition(core::Point localPos) const {
    ensureLineCache();
    if (visualLines_.empty()) {
        return 0;
    }
    core::Rect vp = textViewportLocalRect();
    float lh = effectiveLineHeight();
    float relY = localPos.y - vp.origin.y + scrollOffsetY_;
    int idx = static_cast<int>(std::floor(relY / lh));
    idx = std::clamp(idx, 0, static_cast<int>(visualLines_.size()) - 1);

    const auto& vl = visualLines_[static_cast<size_t>(idx)];
    float relX = localPos.x - vp.origin.x + scrollOffsetX_;
    if (relX <= 0.0f || vl.startByte == vl.endByte) {
        return vl.startByte;
    }

    size_t bestRel = 0;
    float bestDist = std::abs(relX);
    size_t rel = 0;
    size_t sliceLen = vl.endByte - vl.startByte;
    while (rel <= sliceLen) {
        float px = rel < vl.clusterPenX.size() ? vl.clusterPenX[rel] : vl.width;
        float dist = std::abs(relX - px);
        if (dist < bestDist) {
            bestDist = dist;
            bestRel = rel;
        }
        if (rel == sliceLen) {
            break;
        }
        rel = nextUtf8Char(std::string_view(text_).substr(vl.startByte, sliceLen), rel);
    }
    return vl.startByte + bestRel;
}

float TextArea::maxScrollOffsetX() const noexcept {
    if (wordWrap_ == text::WordWrap::Word) {
        return 0.0f;
    }
    ensureLineCache();
    float vpW = textViewportLocalRect().size.width;
    return std::max(0.0f, contentMaxWidth_ - vpW + 8.0f * dpiScale_);
}

float TextArea::maxScrollOffsetY() const noexcept {
    ensureLineCache();
    float vpH = textViewportLocalRect().size.height;
    return std::max(0.0f, contentHeight_ - vpH);
}

void TextArea::clampScrollOffsets() noexcept {
    scrollOffsetX_ = std::clamp(scrollOffsetX_, 0.0f, maxScrollOffsetX());
    scrollOffsetY_ = std::clamp(scrollOffsetY_, 0.0f, maxScrollOffsetY());
}

void TextArea::scrollBy(float deltaX, float deltaY) {
    float prevX = scrollOffsetX_;
    float prevY = scrollOffsetY_;
    scrollOffsetX_ += deltaX;
    scrollOffsetY_ += deltaY;
    clampScrollOffsets();
    if (scrollOffsetX_ != prevX || scrollOffsetY_ != prevY) {
        markPaintDirty();
    }
}

void TextArea::ensureCursorVisible() {
    ensureLineCache();
    if (visualLines_.empty()) {
        return;
    }
    core::Rect vp = textViewportLocalRect();
    if (vp.size.width <= 0.0f || vp.size.height <= 0.0f) {
        return;
    }
    size_t vlIdx = visualLineForByte(cursorIndex_);
    const auto& vl = visualLines_[vlIdx];
    float lh = effectiveLineHeight();
    float caretY = vl.y;
    float caretX = xOffsetInVisualLine(vl, cursorIndex_);

    if (caretY < scrollOffsetY_) {
        scrollOffsetY_ = caretY;
    } else if (caretY + lh > scrollOffsetY_ + vp.size.height) {
        scrollOffsetY_ = caretY + lh - vp.size.height;
    }

    if (wordWrap_ == text::WordWrap::None) {
        float margin = 8.0f * dpiScale_;
        if (caretX < scrollOffsetX_) {
            scrollOffsetX_ = std::max(0.0f, caretX - margin);
        } else if (caretX > scrollOffsetX_ + vp.size.width - margin) {
            scrollOffsetX_ = caretX - vp.size.width + margin;
        }
    }
    clampScrollOffsets();
}

core::Rect TextArea::verticalThumbRect() const {
    float maxY = maxScrollOffsetY();
    if (maxY <= 0.0f) {
        return core::Rect{};
    }
    core::Size sz = layoutRect().size;
    float pad = 3.0f * dpiScale_;
    float trackH = std::max(0.0f, sz.height - pad * 2.0f);
    float vpH = textViewportLocalRect().size.height;
    float ratio = contentHeight_ > 0.0f ? std::min(1.0f, vpH / contentHeight_) : 1.0f;
    float thumbH = std::max(20.0f * dpiScale_, trackH * ratio);
    float availTrack = std::max(0.0f, trackH - thumbH);
    float thumbY = pad + (maxY > 0.0f ? (scrollOffsetY_ / maxY) * availTrack : 0.0f);
    float thumbX = sz.width - scrollbarThickness_ - pad;
    return core::Rect{core::Point{thumbX, thumbY}, core::Size{scrollbarThickness_, thumbH}};
}

core::Rect TextArea::horizontalThumbRect() const {
    float maxX = maxScrollOffsetX();
    if (maxX <= 0.0f) {
        return core::Rect{};
    }
    core::Size sz = layoutRect().size;
    float pad = 3.0f * dpiScale_;
    float gw = gutterWidth();
    float trackW = std::max(0.0f, sz.width - gw - scrollbarThickness_ - pad * 3.0f);
    float vpW = textViewportLocalRect().size.width;
    float totalW = contentMaxWidth_ + 8.0f * dpiScale_;
    float ratio = totalW > 0.0f ? std::min(1.0f, vpW / totalW) : 1.0f;
    float thumbW = std::max(20.0f * dpiScale_, trackW * ratio);
    float availTrack = std::max(0.0f, trackW - thumbW);
    float thumbX = gw + pad + (maxX > 0.0f ? (scrollOffsetX_ / maxX) * availTrack : 0.0f);
    float thumbY = sz.height - scrollbarThickness_ - pad;
    return core::Rect{core::Point{thumbX, thumbY}, core::Size{thumbW, scrollbarThickness_}};
}

void TextArea::deleteSelection() {
    if (!hasSelection() || readOnly_) {
        return;
    }
    size_t start = std::min(cursorIndex_, selectionAnchor_);
    size_t end = std::max(cursorIndex_, selectionAnchor_);
    std::string next = text_;
    next.erase(start, end - start);
    applyEdit(std::move(next), start, start, EditKind::Delete);
}

void TextArea::insertText(std::string_view str, EditKind kind) {
    if (readOnly_) {
        return;
    }
    std::string clean = normalizeNewlines(str);
    size_t start = std::min(cursorIndex_, selectionAnchor_);
    size_t end = std::max(cursorIndex_, selectionAnchor_);

    if (maxLength_ > 0) {
        size_t remainingLen =
            utf8CodepointCount(text_) -
            utf8CodepointCount(std::string_view(text_).substr(start, end - start));
        if (remainingLen >= maxLength_) {
            return;
        }
        size_t allowed = maxLength_ - remainingLen;
        if (utf8CodepointCount(clean) > allowed) {
            clean = std::string(truncateToCodepoints(clean, allowed));
        }
    }

    if (clean.empty() && start == end) {
        return;
    }
    std::string next = text_;
    next.replace(start, end - start, clean);
    size_t newPos = start + clean.size();
    applyEdit(std::move(next), newPos, newPos, kind);
}

void TextArea::setBackgroundColor(const core::Color& color) noexcept {
    overrideBgColor_ = color;
    markPaintDirty();
}

void TextArea::setIdleBorderColor(const core::Color& color) noexcept {
    overrideIdleBorderColor_ = color;
    markPaintDirty();
}

void TextArea::setFocusedBorderColor(const core::Color& color) noexcept {
    overrideFocusedBorderColor_ = color;
    markPaintDirty();
}

void TextArea::setTextColor(const core::Color& color) noexcept {
    overrideTextColor_ = color;
    markPaintDirty();
}

void TextArea::setPlaceholderColor(const core::Color& color) noexcept {
    overridePlaceholderColor_ = color;
    markPaintDirty();
}

void TextArea::setSelectionColor(const core::Color& color) noexcept {
    overrideSelectionColor_ = color;
    markPaintDirty();
}

void TextArea::setCursorColor(const core::Color& color) noexcept {
    overrideCursorColor_ = color;
    markPaintDirty();
}

void TextArea::setCornerRadius(float radius) noexcept {
    cornerRadius_ = std::max(0.0f, radius);
    markPaintDirty();
}

void TextArea::clearColorOverrides() noexcept {
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

core::Color TextArea::effectiveBackgroundColor() const noexcept {
    if (overrideBgColor_) {
        return *overrideBgColor_;
    }
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.surfaceBase;
    }
    return (pal.mode == theme::ThemeMode::Dark) ? pal.surfaceBase : pal.surfaceDefault;
}

core::Color TextArea::effectiveBorderColor() const noexcept {
    const auto& pal = colorPalette();
    if (!isEnabled()) {
        return pal.borderSubtle;
    }
    core::Color idle = overrideIdleBorderColor_.value_or(pal.borderDefault);
    core::Color hoverCol = pal.borderStrong;
    core::Color focused = overrideFocusedBorderColor_.value_or(pal.accentDefault);
    core::Color base = idle.lerp(hoverCol, hoverAnim_.currentValue());
    return base.lerp(focused, focusAnim_.currentValue());
}

core::Color TextArea::effectiveTextColor() const noexcept {
    if (overrideTextColor_) {
        return isEnabled() ? *overrideTextColor_
                           : overrideTextColor_->withAlpha(overrideTextColor_->a * 0.40f);
    }
    const auto& pal = colorPalette();
    return isEnabled() ? pal.textPrimary : pal.textDisabled;
}

core::Color TextArea::effectivePlaceholderColor() const noexcept {
    return overridePlaceholderColor_.value_or(colorPalette().textTertiary);
}

core::Color TextArea::effectiveSelectionColor() const noexcept {
    return overrideSelectionColor_.value_or(colorPalette().accentMuted);
}

core::Color TextArea::effectiveCursorColor() const noexcept {
    return overrideCursorColor_.value_or(colorPalette().accentDefault);
}

void TextArea::cancelActiveInteraction() {
    if (context_ && context_->mouseGrabWidget() == this) {
        context_->releaseMouseGrab();
    }
    bool wasActive = isDraggingSelection_ || isDraggingVThumb_ || isDraggingHThumb_ || isHovered_ ||
                     isHoveringVThumb_ || isHoveringHThumb_;
    isDraggingSelection_ = false;
    isDraggingVThumb_ = false;
    isDraggingHThumb_ = false;
    isHovered_ = false;
    isHoveringVThumb_ = false;
    isHoveringHThumb_ = false;
    hoverAnim_.setCurrentValue(0.0f);
    if (wasActive) {
        markPaintDirty();
    }
}

void TextArea::onMouseEnter() {
    if (!isEnabled()) {
        return;
    }
    isHovered_ = true;
    hoverAnim_.start(hoverAnim_.currentValue(), 1.0f, 0.12f,
                     core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void TextArea::onMouseLeave() {
    isHovered_ = false;
    isHoveringVThumb_ = false;
    isHoveringHThumb_ = false;
    if (isEnabled()) {
        hoverAnim_.start(hoverAnim_.currentValue(), 0.0f, 0.12f,
                         core::animation::EasingCurve::OutCubic);
        if (context_) {
            context_->registerAnimatedWidget(this);
        }
    }
    markPaintDirty();
}

void TextArea::onMouseDown(WidgetMouseEvent& ev) {
    if (!isEnabled() || !ev.isPrimaryButton()) {
        return;
    }
    setFocus(FocusReason::Pointer);

    core::Rect vThumb = verticalThumbRect();
    if (vThumb.size.width > 0.0f && vThumb.contains(ev.position())) {
        isDraggingVThumb_ = true;
        dragStartMouseY_ = ev.position().y;
        dragStartScrollY_ = scrollOffsetY_;
        if (context_) {
            context_->setMouseGrabWidget(this);
        }
        ev.accept();
        markPaintDirty();
        return;
    }

    core::Rect hThumb = horizontalThumbRect();
    if (hThumb.size.width > 0.0f && hThumb.contains(ev.position())) {
        isDraggingHThumb_ = true;
        dragStartMouseX_ = ev.position().x;
        dragStartScrollX_ = scrollOffsetX_;
        if (context_) {
            context_->setMouseGrabWidget(this);
        }
        ev.accept();
        markPaintDirty();
        return;
    }

    size_t hit = hitTestPosition(ev.position());
    auto now = std::chrono::steady_clock::now();
    float tol = 4.0f * dpiScale_;
    float elapsedMs = std::chrono::duration<float, std::milli>(now - lastClickTime_).count();
    if (clickCount_ > 0 && elapsedMs <= kMultiClickMaxMs &&
        std::abs(ev.position().x - lastClickPos_.x) <= tol &&
        std::abs(ev.position().y - lastClickPos_.y) <= tol) {
        clickCount_ = (clickCount_ % 3) + 1;
    } else {
        clickCount_ = 1;
    }
    lastClickTime_ = now;
    lastClickPos_ = ev.position();

    size_t prevC = cursorIndex_;
    size_t prevA = selectionAnchor_;

    if (clickCount_ == 2) {
        auto [ws, we] = wordBoundsAt(text_, hit);
        selectionAnchor_ = ws;
        cursorIndex_ = we;
    } else if (clickCount_ == 3) {
        auto [ps, pe] = paragraphBoundsAt(text_, hit);
        selectionAnchor_ = ps;
        cursorIndex_ = pe;
    } else {
        bool shift = platform::hasModifier(ev.modifiers(), platform::KeyModifier::Shift);
        cursorIndex_ = hit;
        if (!shift) {
            selectionAnchor_ = hit;
        }
        isDraggingSelection_ = true;
        if (context_) {
            context_->setMouseGrabWidget(this);
        }
    }

    desiredCaretX_ = -1.0f;
    resetBlink();
    ensureCursorVisible();
    notifyCursorAndSelection(prevC, prevA);
    ev.accept();
    markPaintDirty();
}

void TextArea::onMouseMove(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    if (isDraggingVThumb_) {
        float maxY = maxScrollOffsetY();
        core::Rect thumb = verticalThumbRect();
        float pad = 3.0f * dpiScale_;
        float trackH = std::max(0.0f, layoutRect().size.height - pad * 2.0f - thumb.size.height);
        if (trackH > 0.0f && maxY > 0.0f) {
            float deltaY = ev.position().y - dragStartMouseY_;
            scrollOffsetY_ = std::clamp(dragStartScrollY_ + (deltaY / trackH) * maxY, 0.0f, maxY);
            markPaintDirty();
        }
        ev.accept();
        return;
    }
    if (isDraggingHThumb_) {
        float maxX = maxScrollOffsetX();
        core::Rect thumb = horizontalThumbRect();
        float pad = 3.0f * dpiScale_;
        float trackW = std::max(0.0f, layoutRect().size.width - gutterWidth() -
                                          scrollbarThickness_ - pad * 3.0f - thumb.size.width);
        if (trackW > 0.0f && maxX > 0.0f) {
            float deltaX = ev.position().x - dragStartMouseX_;
            scrollOffsetX_ = std::clamp(dragStartScrollX_ + (deltaX / trackW) * maxX, 0.0f, maxX);
            markPaintDirty();
        }
        ev.accept();
        return;
    }
    if (isDraggingSelection_) {
        size_t prevC = cursorIndex_;
        size_t prevA = selectionAnchor_;
        cursorIndex_ = hitTestPosition(ev.position());
        desiredCaretX_ = -1.0f;
        ensureCursorVisible();
        notifyCursorAndSelection(prevC, prevA);
        ev.accept();
        markPaintDirty();
        return;
    }

    bool hovV = verticalThumbRect().contains(ev.position());
    bool hovH = horizontalThumbRect().contains(ev.position());
    if (hovV != isHoveringVThumb_ || hovH != isHoveringHThumb_) {
        isHoveringVThumb_ = hovV;
        isHoveringHThumb_ = hovH;
        markPaintDirty();
    }
    bool overText = textViewportLocalRect().contains(ev.position()) && !hovV && !hovH;
    setCursorShape(overText ? platform::CursorShape::IBeam : platform::CursorShape::Arrow);
}

void TextArea::onMouseUp(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        cancelActiveInteraction();
        return;
    }
    if (ev.isPrimaryButton() && (isDraggingSelection_ || isDraggingVThumb_ || isDraggingHThumb_)) {
        isDraggingSelection_ = false;
        isDraggingVThumb_ = false;
        isDraggingHThumb_ = false;
        if (context_ && context_->mouseGrabWidget() == this) {
            context_->releaseMouseGrab();
        }
        ev.accept();
        markPaintDirty();
    }
}

void TextArea::onMouseWheel(WidgetMouseEvent& ev) {
    if (!isEnabled()) {
        return;
    }
    bool shift = platform::hasModifier(ev.modifiers(), platform::KeyModifier::Shift);
    float step = 32.0f * dpiScale_;
    if (shift && maxScrollOffsetX() > 0.0f) {
        scrollBy(-ev.wheelDelta().y * step, 0.0f);
        ev.accept();
    } else if (maxScrollOffsetY() > 0.0f || maxScrollOffsetX() > 0.0f) {
        scrollBy(-ev.wheelDelta().x * step, -ev.wheelDelta().y * step);
        ev.accept();
    }
}

void TextArea::onKeyDown(WidgetKeyEvent& ev) {
    if (!isEnabled() || !ev.isDown()) {
        return;
    }

    bool ctrl = platform::hasModifier(ev.modifiers(), platform::KeyModifier::Control);
    bool shift = platform::hasModifier(ev.modifiers(), platform::KeyModifier::Shift);

    if (ctrl && ev.key() == platform::Key::A) {
        selectAll();
        ev.accept();
        return;
    }
    if (ctrl && ev.key() == platform::Key::C) {
        if (hasSelection()) {
            platform::Clipboard::setText(selectedText());
        }
        ev.accept();
        return;
    }
    if (ctrl && ev.key() == platform::Key::X) {
        if (hasSelection()) {
            platform::Clipboard::setText(selectedText());
            deleteSelection();
        }
        ev.accept();
        return;
    }
    if (ctrl && ev.key() == platform::Key::V) {
        if (!readOnly_) {
            std::string clip = platform::Clipboard::getText().valueOr("");
            if (!clip.empty()) {
                insertText(clip, EditKind::None);
            }
        }
        ev.accept();
        return;
    }
    if (ctrl && ev.key() == platform::Key::Z) {
        if (shift) {
            redo();
        } else {
            undo();
        }
        ev.accept();
        return;
    }
    if (ctrl && ev.key() == platform::Key::Y) {
        redo();
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Tab) {
        if (acceptsTab_ && !readOnly_ && !ctrl) {
            insertText(std::string(static_cast<size_t>(tabWidth_), ' '), EditKind::None);
            ev.accept();
        }
        return;
    }

    if (ev.key() == platform::Key::Enter) {
        if (!readOnly_) {
            insertText("\n", EditKind::None);
        }
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Backspace) {
        if (!readOnly_) {
            if (hasSelection()) {
                deleteSelection();
            } else if (cursorIndex_ > 0) {
                size_t delStart = ctrl ? prevWordBoundary(text_, cursorIndex_)
                                       : prevUtf8Char(text_, cursorIndex_);
                std::string next = text_;
                next.erase(delStart, cursorIndex_ - delStart);
                applyEdit(std::move(next), delStart, delStart,
                          ctrl ? EditKind::None : EditKind::Delete);
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
                size_t delEnd = ctrl ? nextWordBoundary(text_, cursorIndex_)
                                     : nextUtf8Char(text_, cursorIndex_);
                std::string next = text_;
                next.erase(cursorIndex_, delEnd - cursorIndex_);
                applyEdit(std::move(next), cursorIndex_, cursorIndex_,
                          ctrl ? EditKind::None : EditKind::Delete);
            }
        }
        ev.accept();
        return;
    }

    ensureLineCache();
    size_t prevC = cursorIndex_;
    size_t prevA = selectionAnchor_;

    if (ev.key() == platform::Key::Left) {
        if (!shift && hasSelection()) {
            cursorIndex_ = std::min(cursorIndex_, selectionAnchor_);
        } else {
            cursorIndex_ =
                ctrl ? prevWordBoundary(text_, cursorIndex_) : prevUtf8Char(text_, cursorIndex_);
        }
        if (!shift) {
            selectionAnchor_ = cursorIndex_;
        }
        desiredCaretX_ = -1.0f;
        resetBlink();
        ensureCursorVisible();
        notifyCursorAndSelection(prevC, prevA);
        markPaintDirty();
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Right) {
        if (!shift && hasSelection()) {
            cursorIndex_ = std::max(cursorIndex_, selectionAnchor_);
        } else {
            cursorIndex_ =
                ctrl ? nextWordBoundary(text_, cursorIndex_) : nextUtf8Char(text_, cursorIndex_);
        }
        if (!shift) {
            selectionAnchor_ = cursorIndex_;
        }
        desiredCaretX_ = -1.0f;
        resetBlink();
        ensureCursorVisible();
        notifyCursorAndSelection(prevC, prevA);
        markPaintDirty();
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Up || ev.key() == platform::Key::Down ||
        ev.key() == platform::Key::PageUp || ev.key() == platform::Key::PageDown) {
        if (!visualLines_.empty()) {
            size_t vlIdx = visualLineForByte(cursorIndex_);
            if (desiredCaretX_ < 0.0f) {
                desiredCaretX_ = xOffsetInVisualLine(visualLines_[vlIdx], cursorIndex_);
            }
            int pageLines = std::max(
                1, static_cast<int>(textViewportLocalRect().size.height / effectiveLineHeight()));
            int delta = (ev.key() == platform::Key::Up)       ? -1
                        : (ev.key() == platform::Key::Down)   ? 1
                        : (ev.key() == platform::Key::PageUp) ? -pageLines
                                                              : pageLines;
            int targetIdx = std::clamp(static_cast<int>(vlIdx) + delta, 0,
                                       static_cast<int>(visualLines_.size()) - 1);
            const auto& targetVl = visualLines_[static_cast<size_t>(targetIdx)];
            core::Rect vp = textViewportLocalRect();
            core::Point hitPt{vp.origin.x + desiredCaretX_ - scrollOffsetX_,
                              vp.origin.y + targetVl.y - scrollOffsetY_ +
                                  effectiveLineHeight() * 0.5f};
            cursorIndex_ = hitTestPosition(hitPt);
            if (!shift) {
                selectionAnchor_ = cursorIndex_;
            }
            resetBlink();
            ensureCursorVisible();
            notifyCursorAndSelection(prevC, prevA);
            markPaintDirty();
        }
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::Home) {
        if (ctrl || visualLines_.empty()) {
            cursorIndex_ = 0;
        } else {
            cursorIndex_ = visualLines_[visualLineForByte(cursorIndex_)].startByte;
        }
        if (!shift) {
            selectionAnchor_ = cursorIndex_;
        }
        desiredCaretX_ = -1.0f;
        resetBlink();
        ensureCursorVisible();
        notifyCursorAndSelection(prevC, prevA);
        markPaintDirty();
        ev.accept();
        return;
    }

    if (ev.key() == platform::Key::End) {
        if (ctrl || visualLines_.empty()) {
            cursorIndex_ = text_.size();
        } else {
            cursorIndex_ = visualLines_[visualLineForByte(cursorIndex_)].endByte;
        }
        if (!shift) {
            selectionAnchor_ = cursorIndex_;
        }
        desiredCaretX_ = -1.0f;
        resetBlink();
        ensureCursorVisible();
        notifyCursorAndSelection(prevC, prevA);
        markPaintDirty();
        ev.accept();
        return;
    }

    uint32_t cp = ev.textCodepoint();
    if (!readOnly_ && !ctrl && cp >= 32 && cp != 127) {
        insertText(codepointToUtf8(cp), EditKind::InsertChar);
        ev.accept();
    }
}

void TextArea::onFocusGained() {
    resetBlink();
    focusAnim_.start(focusAnim_.currentValue(), 1.0f, 0.14f,
                     core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void TextArea::onFocusLost() {
    cancelActiveInteraction();
    cursorVisible_ = false;
    focusAnim_.start(focusAnim_.currentValue(), 0.0f, 0.14f,
                     core::animation::EasingCurve::OutCubic);
    if (context_) {
        context_->registerAnimatedWidget(this);
    }
    markPaintDirty();
}

void TextArea::onContextAttached() {
    if (!hasCustomDpiScale_) {
        applyDpiScale(contextDpiScale());
    }
}

void TextArea::onThemeChanged() {
    resolvedDefaultFont_.reset();
    invalidateLineCache();
    placeholderCacheValid_ = false;
    markPaintDirty();
}

void TextArea::onEnabledChanged() {
    if (!isEnabled()) {
        cancelActiveInteraction();
        setCursorShape(platform::CursorShape::Arrow);
    } else {
        setCursorShape(platform::CursorShape::IBeam);
    }
    markPaintDirty();
}

void TextArea::updateAnimations(float deltaSeconds) {
    bool dirty = false;
    if (hoverAnim_.isRunning()) {
        hoverAnim_.update(deltaSeconds);
        dirty = true;
    }
    if (focusAnim_.isRunning()) {
        focusAnim_.update(deltaSeconds);
        dirty = true;
    }
    if (hasFocus() && isEnabled()) {
        cursorBlinkTimer_ += deltaSeconds;
        if (cursorBlinkTimer_ >= 0.53f) {
            cursorBlinkTimer_ -= 0.53f;
            cursorVisible_ = !cursorVisible_;
            dirty = true;
        }
    }
    if (dirty) {
        markPaintDirty();
    }
}

bool TextArea::hasActiveAnimations() const noexcept {
    return hoverAnim_.isRunning() || focusAnim_.isRunning() || (hasFocus() && isEnabled());
}

void TextArea::paint(renderer::PaintContext& ctx) {
    core::Rect sRect = screenRect();
    if (sRect.size.width <= 0.0f || sRect.size.height <= 0.0f) {
        return;
    }

    float scale = dpiScale_ > 0.0f ? dpiScale_ : 1.0f;
    float radius = cornerRadius_ * scale;
    float radii[4] = {radius, radius, radius, radius};
    float borderW = std::max(1.0f, std::round(1.0f * scale));
    const auto& pal = colorPalette();

    ctx.drawBox(sRect, effectiveBackgroundColor(), effectiveBorderColor(), borderW, radii);

    if (hasFocus() && isEnabled()) {
        Widget::paintFocusRing(ctx, sRect, radius);
    }

    ensureLineCache();

    float gw = gutterWidth();
    if (showLineNumbers_ && gw > 0.0f) {
        core::Rect gutterScreen{sRect.origin, core::Size{gw, sRect.size.height}};
        ctx.pushClip(gutterScreen);
        ctx.drawLine(core::Point{sRect.origin.x + gw - borderW * 0.5f, sRect.origin.y + borderW},
                     core::Point{sRect.origin.x + gw - borderW * 0.5f,
                                 sRect.origin.y + sRect.size.height - borderW},
                     pal.borderSubtle, borderW);

        auto f = effectiveFont();
        if (f) {
            float padY = kPaddingY * scale;
            float lh = effectiveLineHeight();
            for (const auto& vl : visualLines_) {
                if (!vl.isFirstInParagraph) {
                    continue;
                }
                float lineTop = vl.y - scrollOffsetY_;
                if (lineTop + lh < 0.0f || lineTop > sRect.size.height) {
                    continue;
                }
                std::string numStr = std::to_string(vl.logicalLine + 1);
                auto numLayout = text::TextLayout::create(numStr, f);
                float numX = sRect.origin.x + gw - 8.0f * scale - numLayout.size().width;
                float numY = sRect.origin.y + padY + lineTop + (lh - f->lineHeight()) * 0.5f;
                ctx.drawTextLayout(core::Point{numX, numY}, numLayout, pal.textTertiary);
            }
        }
        ctx.popClip();
    }

    core::Rect vpLocal = textViewportLocalRect();
    core::Rect vpScreen{
        core::Point{sRect.origin.x + vpLocal.origin.x, sRect.origin.y + vpLocal.origin.y},
        vpLocal.size};
    ctx.pushClip(vpScreen);

    auto f = effectiveFont();
    float lh = effectiveLineHeight();

    if (text_.empty()) {
        ensurePlaceholderCache();
        if (cachedPlaceholderLayout_) {
            ctx.drawTextLayout(vpScreen.origin, *cachedPlaceholderLayout_,
                               effectivePlaceholderColor());
        }
    } else {
        size_t selStart = std::min(cursorIndex_, selectionAnchor_);
        size_t selEnd = std::max(cursorIndex_, selectionAnchor_);
        bool hasSel = hasSelection() && isEnabled();

        for (const auto& vl : visualLines_) {
            float lineTop = vl.y - scrollOffsetY_;
            if (lineTop + lh < 0.0f || lineTop > vpLocal.size.height) {
                continue;
            }
            core::Point origin{vpScreen.origin.x - scrollOffsetX_, vpScreen.origin.y + lineTop};

            if (hasSel && selEnd > vl.startByte && selStart <= vl.endByte) {
                size_t s0 = std::max(selStart, vl.startByte);
                size_t s1 = std::min(selEnd, vl.endByte);
                float x0 = xOffsetInVisualLine(vl, s0);
                float x1 = xOffsetInVisualLine(vl, s1);
                if (selEnd > vl.endByte && s1 == vl.endByte) {
                    x1 += 4.0f * scale;
                }
                if (x1 > x0) {
                    ctx.drawRect(
                        core::Rect{core::Point{origin.x + x0, origin.y}, core::Size{x1 - x0, lh}},
                        effectiveSelectionColor());
                }
            }

            if (vl.layout) {
                ctx.drawTextLayout(origin, *vl.layout, effectiveTextColor());
            }
        }
    }

    if (hasFocus() && isEnabled() && !readOnly_ && cursorVisible_) {
        size_t vlIdx = visualLineForByte(cursorIndex_);
        if (vlIdx < visualLines_.size()) {
            const auto& vl = visualLines_[vlIdx];
            float caretX =
                vpScreen.origin.x - scrollOffsetX_ + xOffsetInVisualLine(vl, cursorIndex_);
            float caretY = vpScreen.origin.y + vl.y - scrollOffsetY_;
            float caretH = f ? f->lineHeight() : lh;
            float caretTop = caretY + (lh - caretH) * 0.5f;
            float caretW = std::max(1.5f, 1.5f * scale);
            ctx.drawRect(core::Rect{core::Point{caretX, caretTop}, core::Size{caretW, caretH}},
                         effectiveCursorColor());
        }
    }

    ctx.popClip();

    auto paintThumb = [&](const core::Rect& localThumb, bool hovered, bool dragging) {
        if (localThumb.size.width <= 0.0f || localThumb.size.height <= 0.0f) {
            return;
        }
        core::Rect screenThumb{
            core::Point{sRect.origin.x + localThumb.origin.x, sRect.origin.y + localThumb.origin.y},
            localThumb.size};
        core::Color restCol = pal.mode == theme::ThemeMode::Dark
                                  ? core::Color{1.0f, 1.0f, 1.0f, 0.22f}
                                  : core::Color{0.0f, 0.0f, 0.0f, 0.22f};
        core::Color thumbCol =
            dragging ? pal.textSecondary : (hovered ? pal.textTertiary : restCol);
        float tr = scrollbarThickness_ * 0.5f;
        float trRadii[4] = {tr, tr, tr, tr};
        ctx.drawRoundedRect(screenThumb, thumbCol, trRadii);
    };

    paintThumb(verticalThumbRect(), isHoveringVThumb_, isDraggingVThumb_);
    paintThumb(horizontalThumbRect(), isHoveringHThumb_, isDraggingHThumb_);
}

} // namespace northern::widgets
