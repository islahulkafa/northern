#pragma once

#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace northern::widgets {

enum class SortOrder : uint8_t { None, Ascending, Descending };

struct HeaderSection {
    std::string title{};
    float width{100.0f};
    float minWidth{40.0f};
    bool visible{true};
    SortOrder sortOrder{SortOrder::None};
    text::TextAlignment alignment{text::TextAlignment::Left};
};

class HeaderView : public Widget {
public:
    static constexpr float kDefaultHeaderHeight = 28.0f;
    static constexpr float kDividerHitRadius = 4.0f;

    explicit HeaderView(std::shared_ptr<text::Font> font = nullptr);
    ~HeaderView() override = default;

    HeaderView(HeaderView&&) noexcept = default;
    HeaderView& operator=(HeaderView&&) noexcept = default;

    void addSection(std::string title, float width = 100.0f, float minWidth = 40.0f);
    void insertSection(size_t index, std::string title, float width = 100.0f,
                       float minWidth = 40.0f);
    void removeSection(size_t index);
    void clearSections();

    [[nodiscard]] size_t sectionCount() const noexcept { return sections_.size(); }
    [[nodiscard]] const std::vector<HeaderSection>& sections() const noexcept { return sections_; }

    void setSectionTitle(size_t index, std::string title);
    [[nodiscard]] std::string sectionTitle(size_t index) const;

    void setSectionWidth(size_t index, float width);
    [[nodiscard]] float sectionWidth(size_t index) const;
    [[nodiscard]] float sectionPosition(size_t index) const;

    void setSectionSortOrder(size_t index, SortOrder order);
    [[nodiscard]] SortOrder sectionSortOrder(size_t index) const;
    [[nodiscard]] int sortedSection() const noexcept;

    void setStretchLastSection(bool stretch) noexcept;
    [[nodiscard]] bool stretchLastSection() const noexcept { return stretchLastSection_; }

    void setHorizontalOffset(float offset) noexcept;
    [[nodiscard]] float horizontalOffset() const noexcept { return horizontalOffset_; }

    [[nodiscard]] float totalWidth() const noexcept;
    [[nodiscard]] int sectionAt(float x) const noexcept;
    [[nodiscard]] int dividerAt(float x) const noexcept;

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    core::Signal<size_t, float> onSectionResized;
    core::Signal<size_t, SortOrder> onSortChanged;

    void paint(renderer::PaintContext& ctx) override;

    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onMouseLeave() override;
    void onContextAttached() override;
    void onThemeChanged() override;
    void onEnabledChanged() override;

private:
    struct SectionTextCache {
        std::optional<text::TextLayout> layout{std::nullopt};
        float maxWidth{-1.0f};
        bool valid{false};
    };

    void applyDpiScale(float scale);
    void cancelActiveInteraction();
    void invalidateSectionCaches() noexcept;
    void ensureSectionLayout(size_t index, float maxWidth) const;
    float resolvedSectionWidth(size_t index) const;

    std::vector<HeaderSection> sections_{};
    mutable std::vector<SectionTextCache> sectionCaches_{};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
    bool stretchLastSection_{true};
    float horizontalOffset_{0.0f};

    int hoveredSection_{-1};
    int pressedSection_{-1};
    int hoveredDivider_{-1};
    bool isDraggingDivider_{false};
    int dragDivider_{-1};
    float dragStartMouseX_{0.0f};
    float dragStartWidth_{0.0f};
};

} // namespace northern::widgets
