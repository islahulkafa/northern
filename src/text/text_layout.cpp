#include "text/text_layout.hpp"
#include "text/text_shaper.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace northern::text {

TextLayout TextLayout::create(std::string_view text, const std::shared_ptr<Font>& font,
                              float maxWidth, TextAlignment alignment) {
    TextLayout layout;
    layout.font_ = font;
    layout.alignment_ = alignment;

    if (text.empty() || !font) {
        return layout;
    }

    float currentY = 0.0f;
    float maxLineWidth = 0.0f;
    float lineHeight = font->lineHeight();
    float ascender = font->ascender();

    auto spaceGlyphs = TextShaper::shape(*font, " ");
    float spaceWidth = spaceGlyphs.empty() ? (font->pointSize() * 0.25f) : spaceGlyphs[0].advance.x;

    // Helper to process and push a finalized line of text
    auto pushLine = [&](const std::string& lineText) {
        TextLine line;
        if (lineText.empty()) {
            line.bounds = core::Rect{0.0f, currentY, 0.0f, lineHeight};
            line.baseline = currentY + ascender;
            layout.lines_.push_back(std::move(line));
            currentY += lineHeight;
            return;
        }

        auto glyphs = TextShaper::shape(*font, lineText);
        float lineWidth = 0.0f;
        for (const auto& g : glyphs) {
            lineWidth += g.advance.x;
        }

        float startX = 0.0f;
        float availableWidth = (maxWidth > 0.0f) ? maxWidth : lineWidth;
        if (alignment == TextAlignment::Center) {
            startX = std::round(std::max(0.0f, (availableWidth - lineWidth) * 0.5f));
        } else if (alignment == TextAlignment::Right) {
            startX = std::round(std::max(0.0f, availableWidth - lineWidth));
        }

        for (auto& g : glyphs) {
            g.position.x += startX;
            g.position.y += currentY + ascender;
        }

        line.bounds = core::Rect{startX, currentY, lineWidth, lineHeight};
        line.baseline = currentY + ascender;
        line.glyphs = std::move(glyphs);

        maxLineWidth = std::max(maxLineWidth, lineWidth);
        layout.lines_.push_back(std::move(line));
        currentY += lineHeight;
    };

    // Split text into paragraphs by newline
    size_t start = 0;
    while (start <= text.size()) {
        size_t newlinePos = text.find_first_of("\r\n", start);
        std::string_view paragraph;
        if (newlinePos == std::string_view::npos) {
            paragraph = text.substr(start);
            start = text.size() + 1; // Terminate
        } else {
            paragraph = text.substr(start, newlinePos - start);
            if (text[newlinePos] == '\r' && newlinePos + 1 < text.size() &&
                text[newlinePos + 1] == '\n') {
                start = newlinePos + 2;
            } else {
                start = newlinePos + 1;
            }
        }

        if (maxWidth <= 0.0f || paragraph.empty()) {
            pushLine(std::string(paragraph));
            continue;
        }

        // Word wrap paragraph
        std::string currentLine;
        float currentLineWidth = 0.0f;

        size_t wordStart = 0;
        while (wordStart < paragraph.size()) {
            // Skip leading spaces for the next word
            while (wordStart < paragraph.size() &&
                   (paragraph[wordStart] == ' ' || paragraph[wordStart] == '\t')) {
                ++wordStart;
            }
            if (wordStart >= paragraph.size()) {
                break;
            }

            size_t wordEnd = paragraph.find_first_of(" \t", wordStart);
            if (wordEnd == std::string_view::npos) {
                wordEnd = paragraph.size();
            }

            std::string_view word = paragraph.substr(wordStart, wordEnd - wordStart);
            auto wordGlyphs = TextShaper::shape(*font, word);
            float wordWidth = 0.0f;
            for (const auto& g : wordGlyphs) {
                wordWidth += g.advance.x;
            }

            if (currentLine.empty()) {
                currentLine = std::string(word);
                currentLineWidth = wordWidth;
            } else {
                float testWidth = currentLineWidth + spaceWidth + wordWidth;
                if (testWidth > maxWidth) {
                    pushLine(currentLine);
                    currentLine = std::string(word);
                    currentLineWidth = wordWidth;
                } else {
                    currentLine += " ";
                    currentLine += word;
                    currentLineWidth = testWidth;
                }
            }

            wordStart = wordEnd;
        }

        if (!currentLine.empty()) {
            pushLine(currentLine);
        }
    }

    layout.size_ = core::Size{maxLineWidth, currentY};
    return layout;
}

} // namespace northern::text
