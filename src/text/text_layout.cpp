#include "text/text_layout.hpp"
#include "text/text_shaper.hpp"
#include <algorithm>
#include <cmath>

namespace northern::text {

namespace {

std::vector<ShapedGlyph> shapeEllipsis(const Font& font, float& outWidth) {
    auto glyphs = TextShaper::shape(font, "\xE2\x80\xA6"); // U+2026 HORIZONTAL ELLIPSIS
    if (glyphs.empty() || glyphs[0].glyphId == 0) {
        glyphs = TextShaper::shape(font, "...");
    }
    outWidth = 0.0f;
    for (const auto& g : glyphs) {
        outWidth += g.advance.x;
    }
    return glyphs;
}

bool isWhitespaceCluster(std::string_view text, uint32_t cluster) noexcept {
    if (cluster >= text.size()) {
        return false;
    }
    char c = text[cluster];
    return c == ' ' || c == '\t';
}

} // namespace

TextLayout TextLayout::create(std::string_view text, const std::shared_ptr<Font>& font,
                              float maxWidth, TextAlignment alignment) {
    TextLayoutOptions opts{};
    opts.maxWidth = maxWidth;
    opts.alignment = alignment;
    opts.wrapMode = WordWrap::Word;
    opts.overflow = TextOverflow::Clip;
    return create(text, font, opts);
}

TextLayout TextLayout::create(std::string_view text, const std::shared_ptr<Font>& font,
                              const TextLayoutOptions& options) {
    TextLayout layout;
    layout.font_ = font;
    layout.alignment_ = options.alignment;

    if (text.empty() || !font) {
        return layout;
    }

    float lineHeightMult =
        options.lineHeightMultiplier > 0.0f ? options.lineHeightMultiplier : 1.0f;
    float lineHeight = font->lineHeight() * lineHeightMult;
    float ascender = font->ascender() + (lineHeight - font->lineHeight()) * 0.5f;
    float currentY = 0.0f;
    float maxLineWidth = 0.0f;

    std::vector<ShapedGlyph> cachedEllipsis;
    float ellipsisWidth = 0.0f;
    bool ellipsisReady = false;
    auto ensureEllipsis = [&]() {
        if (!ellipsisReady) {
            cachedEllipsis = shapeEllipsis(*font, ellipsisWidth);
            ellipsisReady = true;
        }
    };

    auto finalizeLine = [&](const std::vector<ShapedGlyph>& srcGlyphs, size_t startIdx,
                            size_t endIdx, float penStart, float lineWidth, bool addEllipsis) {
        TextLine line;
        size_t glyphCount = endIdx > startIdx ? (endIdx - startIdx) : 0;
        line.glyphs.reserve(glyphCount + (addEllipsis ? 3 : 0));

        for (size_t i = startIdx; i < endIdx; ++i) {
            ShapedGlyph g = srcGlyphs[i];
            g.position.x -= penStart;
            line.glyphs.push_back(g);
        }

        if (addEllipsis) {
            ensureEllipsis();
            for (const auto& eg : cachedEllipsis) {
                ShapedGlyph g = eg;
                g.position.x += lineWidth;
                line.glyphs.push_back(g);
            }
            lineWidth += ellipsisWidth;
            layout.truncated_ = true;
        }

        float startX = 0.0f;
        float availableWidth = (options.maxWidth > 0.0f) ? options.maxWidth : lineWidth;
        if (options.alignment == TextAlignment::Center) {
            startX = std::round(std::max(0.0f, (availableWidth - lineWidth) * 0.5f));
        } else if (options.alignment == TextAlignment::Right) {
            startX = std::round(std::max(0.0f, availableWidth - lineWidth));
        }

        for (auto& g : line.glyphs) {
            g.position.x += startX;
            g.position.y += currentY + ascender;
        }

        line.bounds = core::Rect{startX, currentY, lineWidth, lineHeight};
        line.baseline = currentY + ascender;
        layout.lines_.push_back(std::move(line));

        maxLineWidth = std::max(maxLineWidth, lineWidth);
        currentY += lineHeight;
    };

    size_t start = 0;
    while (start <= text.size()) {
        size_t newlinePos = text.find_first_of("\r\n", start);
        std::string_view paragraph;
        bool hasMoreParagraphs = false;
        if (newlinePos == std::string_view::npos) {
            paragraph = text.substr(start);
            start = text.size() + 1;
        } else {
            paragraph = text.substr(start, newlinePos - start);
            if (text[newlinePos] == '\r' && newlinePos + 1 < text.size() &&
                text[newlinePos + 1] == '\n') {
                start = newlinePos + 2;
            } else {
                start = newlinePos + 1;
            }
            hasMoreParagraphs = (start <= text.size());
        }

        if (options.maxLines > 0 && layout.lines_.size() >= options.maxLines) {
            layout.truncated_ = true;
            break;
        }

        if (paragraph.empty()) {
            TextLine line;
            line.bounds = core::Rect{0.0f, currentY, 0.0f, lineHeight};
            line.baseline = currentY + ascender;
            layout.lines_.push_back(std::move(line));
            currentY += lineHeight;
            continue;
        }

        auto glyphs = TextShaper::shape(*font, paragraph);
        if (glyphs.empty()) {
            continue;
        }

        std::vector<float> penX(glyphs.size() + 1, 0.0f);
        for (size_t i = 0; i < glyphs.size(); ++i) {
            penX[i + 1] = penX[i] + glyphs[i].advance.x;
        }

        auto trimForEllipsis = [&](size_t lineStart, size_t& outEnd, float& outWidth) {
            ensureEllipsis();
            float basePen = penX[lineStart];
            float budget = std::max(0.0f, options.maxWidth - ellipsisWidth);
            outEnd = lineStart;
            for (size_t i = lineStart; i < glyphs.size(); ++i) {
                if (penX[i + 1] - basePen <= budget) {
                    outEnd = i + 1;
                } else {
                    break;
                }
            }
            while (outEnd > lineStart &&
                   isWhitespaceCluster(paragraph, glyphs[outEnd - 1].cluster)) {
                --outEnd;
            }
            outWidth = penX[outEnd] - basePen;
        };

        if (options.wrapMode == WordWrap::None || options.maxWidth <= 0.0f) {
            float totalW = penX[glyphs.size()];
            bool isLastAllowedLine =
                (options.maxLines > 0 && layout.lines_.size() + 1 == options.maxLines);
            bool overflowsWidth = (options.maxWidth > 0.0f && totalW > options.maxWidth);

            if (options.overflow == TextOverflow::Ellipsis && options.maxWidth > 0.0f &&
                (overflowsWidth || (isLastAllowedLine && hasMoreParagraphs))) {
                size_t endIdx = 0;
                float trimmedW = 0.0f;
                trimForEllipsis(0, endIdx, trimmedW);
                finalizeLine(glyphs, 0, endIdx, 0.0f, trimmedW, true);
            } else {
                finalizeLine(glyphs, 0, glyphs.size(), 0.0f, totalW, false);
                if (isLastAllowedLine && hasMoreParagraphs) {
                    layout.truncated_ = true;
                }
            }
            if (isLastAllowedLine) {
                break;
            }
            continue;
        }

        // Single-pass cluster-based word wrapping
        size_t cursor = 0;
        while (cursor < glyphs.size()) {
            while (cursor < glyphs.size() &&
                   isWhitespaceCluster(paragraph, glyphs[cursor].cluster)) {
                ++cursor;
            }
            if (cursor >= glyphs.size()) {
                break;
            }

            size_t lineStart = cursor;
            float linePenStart = penX[lineStart];
            size_t lastEnd = lineStart;
            bool hasWord = false;
            size_t scan = lineStart;

            while (scan < glyphs.size()) {
                while (scan < glyphs.size() &&
                       isWhitespaceCluster(paragraph, glyphs[scan].cluster)) {
                    ++scan;
                }
                if (scan >= glyphs.size()) {
                    break;
                }

                size_t wordEnd = scan;
                while (wordEnd < glyphs.size() &&
                       !isWhitespaceCluster(paragraph, glyphs[wordEnd].cluster)) {
                    ++wordEnd;
                }

                float candidateW = penX[wordEnd] - linePenStart;
                if (!hasWord || candidateW <= options.maxWidth) {
                    lastEnd = wordEnd;
                    hasWord = true;
                    scan = wordEnd;
                } else {
                    break;
                }
            }

            size_t nextCursor = lastEnd;
            while (nextCursor < glyphs.size() &&
                   isWhitespaceCluster(paragraph, glyphs[nextCursor].cluster)) {
                ++nextCursor;
            }
            bool moreContentFollows = (nextCursor < glyphs.size()) || hasMoreParagraphs;
            bool isLastAllowedLine =
                (options.maxLines > 0 && layout.lines_.size() + 1 == options.maxLines);

            if (isLastAllowedLine && moreContentFollows) {
                if (options.overflow == TextOverflow::Ellipsis) {
                    size_t endIdx = lineStart;
                    float trimmedW = 0.0f;
                    trimForEllipsis(lineStart, endIdx, trimmedW);
                    finalizeLine(glyphs, lineStart, endIdx, linePenStart, trimmedW, true);
                } else {
                    finalizeLine(glyphs, lineStart, lastEnd, linePenStart,
                                 penX[lastEnd] - linePenStart, false);
                    layout.truncated_ = true;
                }
                cursor = glyphs.size();
                break;
            }

            finalizeLine(glyphs, lineStart, lastEnd, linePenStart, penX[lastEnd] - linePenStart,
                         false);
            cursor = nextCursor;
        }

        if (options.maxLines > 0 && layout.lines_.size() >= options.maxLines && hasMoreParagraphs) {
            layout.truncated_ = true;
            break;
        }
    }

    layout.size_ = core::Size{maxLineWidth, currentY};
    return layout;
}

} // namespace northern::text
