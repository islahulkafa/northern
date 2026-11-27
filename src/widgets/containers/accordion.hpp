#pragma once

#include "core/animation/property_animation.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace northern::widgets {

class Container;

enum class SectionHeaderSize {
    Standard, // 38px height, 14px indicator, 11.5pt title
    Compact   // 30px height, 12px indicator, 10.5pt title
};

enum class IndicatorPlacement {
    Leading, // Chevron on left of title
    Trailing // Chevron on right of header
};

enum class AccordionStyle : uint8_t {
    Grouped,   // Unified rounded Optical Zinc card with inset hairline dividers
    Separated, // Individual rounded Zinc cards with vertical gap
    Flush      // Edge-to-edge borderless inspector sections
};

class CollapsibleSection : public Widget {
public:
    static constexpr float kHeaderHeightStandard = 38.0f;
    static constexpr float kHeaderHeightCompact = 30.0f;
    static constexpr float kHeaderHeight = 38.0f;

    explicit CollapsibleSection(std::string title = {}, bool expanded = true,
                                std::shared_ptr<text::Font> font = nullptr,
                                SectionHeaderSize headerSize = SectionHeaderSize::Standard);
    ~CollapsibleSection() override = default;

    CollapsibleSection(CollapsibleSection&&) noexcept = default;
    CollapsibleSection& operator=(CollapsibleSection&&) noexcept = default;

    [[nodiscard]] const std::string& title() const noexcept { return title_; }
    void setTitle(std::string title);

    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    [[nodiscard]] SectionHeaderSize headerSize() const noexcept { return headerSize_; }
    void setHeaderSize(SectionHeaderSize size) noexcept;
    [[nodiscard]] float headerHeight() const noexcept;

    [[nodiscard]] IndicatorPlacement indicatorPlacement() const noexcept {
        return indicatorPlacement_;
    }
    void setIndicatorPlacement(IndicatorPlacement placement) noexcept;

    void setCardStyled(bool cardStyled) noexcept;
    [[nodiscard]] bool isCardStyled() const noexcept { return cardStyled_; }

    void setShowRowDividers(bool show) noexcept;
    [[nodiscard]] bool showRowDividers() const noexcept { return showRowDividers_; }

    [[nodiscard]] bool isExpanded() const noexcept { return isExpanded_; }
    void setExpanded(bool expanded, bool animated = true);
    void toggle() { setExpanded(!isExpanded_); }

    template <typename T>
    T* setContent(std::unique_ptr<T> content) {
        T* raw = content.get();
        setContentInternal(std::move(content));
        return raw;
    }

    template <typename T>
    T* addContent(std::unique_ptr<T> item) {
        T* raw = item.get();
        addContentInternal(std::move(item));
        return raw;
    }

    Container* addRow(std::string label, std::unique_ptr<Widget> trailing = nullptr);

    [[nodiscard]] Widget* content() const noexcept { return content_; }
    [[nodiscard]] Container* body() const noexcept { return body_; }

    template <typename T>
    T* setHeaderAccessory(std::unique_ptr<T> accessory) {
        T* raw = accessory.get();
        setHeaderAccessoryInternal(std::move(accessory));
        return raw;
    }
    [[nodiscard]] Widget* headerAccessory() const noexcept { return headerAccessory_; }

    void setContentPadding(const layout::RectLength& padding) noexcept;
    [[nodiscard]] const layout::RectLength& contentPadding() const noexcept {
        return contentPadding_;
    }

    [[nodiscard]] bool clipsChildren() const noexcept override { return true; }

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

    void onContextAttached() override;
    void onThemeChanged() override;
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;
    void onFocusLost() override;

    core::Signal<bool> onToggled;

private:
    void setContentInternal(std::unique_ptr<Widget> content);
    void addContentInternal(std::unique_ptr<Widget> item);
    void setHeaderAccessoryInternal(std::unique_ptr<Widget> accessory);
    void applyDpiMetrics(float scale);
    void applySizeMetrics() noexcept;
    void updateAccessoryLayout() noexcept;
    void updateHoverAnimation(bool hovered);
    float calculateTargetHeight();

    std::string title_{};
    std::shared_ptr<text::Font> font_{};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};

    SectionHeaderSize headerSize_{SectionHeaderSize::Standard};
    IndicatorPlacement indicatorPlacement_{IndicatorPlacement::Trailing};
    bool cardStyled_{false};
    bool showRowDividers_{true};
    bool hasCustomPadding_{false};

    mutable std::optional<text::TextLayout> cachedTitleLayout_{};
    mutable bool isTitleLayoutDirty_{true};

    bool isExpanded_{true};
    bool isHoveredHeader_{false};
    bool isPressedHeader_{false};
    bool wasExpandAnimRunning_{false};

    float targetBodyHeight_{0.0f};
    core::animation::PropertyAnimation<float> expandAnim_{1.0f};
    core::animation::PropertyAnimation<float> hoverAnim_{0.0f};

    layout::RectLength contentPadding_{
        .left = layout::Length::points(20.0f),
        .top = layout::Length::points(2.0f),
        .right = layout::Length::points(16.0f),
        .bottom = layout::Length::points(2.0f),
    };

    Container* body_{nullptr};
    Widget* content_{nullptr};
    Widget* headerAccessory_{nullptr};
};

using DisclosureGroup = CollapsibleSection;

class Accordion : public Widget {
public:
    explicit Accordion(bool singleExpanded = false,
                       SectionHeaderSize headerSize = SectionHeaderSize::Standard,
                       AccordionStyle style = AccordionStyle::Grouped);
    ~Accordion() override = default;

    Accordion(Accordion&&) noexcept = default;
    Accordion& operator=(Accordion&&) noexcept = default;

    CollapsibleSection* addSection(std::unique_ptr<CollapsibleSection> section);
    CollapsibleSection* addSection(std::string title, std::unique_ptr<Widget> content = nullptr,
                                   bool expanded = false);

    void removeSection(size_t index);
    void clearSections();

    [[nodiscard]] size_t sectionCount() const noexcept { return sections_.size(); }
    [[nodiscard]] CollapsibleSection* sectionAt(size_t index) const;
    [[nodiscard]] std::optional<size_t>
    indexOfSection(const CollapsibleSection* section) const noexcept;

    [[nodiscard]] bool isSingleExpanded() const noexcept { return singleExpanded_; }
    void setSingleExpanded(bool singleExpanded);

    [[nodiscard]] SectionHeaderSize headerSize() const noexcept { return headerSize_; }
    void setHeaderSize(SectionHeaderSize size) noexcept;

    [[nodiscard]] AccordionStyle accordionStyle() const noexcept { return accordionStyle_; }
    void setAccordionStyle(AccordionStyle style);

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    [[nodiscard]] bool clipsChildren() const noexcept override {
        return accordionStyle_ == AccordionStyle::Grouped;
    }

    void paint(renderer::PaintContext& ctx) override;
    void onContextAttached() override;
    void onThemeChanged() override;

    void focusNextSection(CollapsibleSection* current);
    void focusPrevSection(CollapsibleSection* current);
    void focusFirstSection();
    void focusLastSection();

    core::Signal<size_t, bool> onSectionToggled;

private:
    void applyDpiMetrics(float scale);
    void applyStyleToSections();

    bool singleExpanded_{false};
    SectionHeaderSize headerSize_{SectionHeaderSize::Standard};
    AccordionStyle accordionStyle_{AccordionStyle::Grouped};
    float dpiScale_{1.0f};
    bool hasCustomDpiScale_{false};
    std::vector<CollapsibleSection*> sections_{};
};

} // namespace northern::widgets
