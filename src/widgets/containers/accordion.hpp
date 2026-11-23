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
    Standard, // 32px height, 14px indicator, 12pt title
    Compact   // 28px height, 12px indicator, 11pt title
};

enum class IndicatorPlacement {
    Leading,  // Chevron on left of title
    Trailing  // Chevron on right of header
};

class CollapsibleSection : public Widget {
public:
    static constexpr float kHeaderHeightStandard = 32.0f;
    static constexpr float kHeaderHeightCompact = 28.0f;
    static constexpr float kHeaderHeight = 32.0f;

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

    [[nodiscard]] SectionHeaderSize headerSize() const noexcept { return headerSize_; }
    void setHeaderSize(SectionHeaderSize size) noexcept;
    [[nodiscard]] float headerHeight() const noexcept;

    [[nodiscard]] IndicatorPlacement indicatorPlacement() const noexcept { return indicatorPlacement_; }
    void setIndicatorPlacement(IndicatorPlacement placement) noexcept;

    [[nodiscard]] bool isExpanded() const noexcept { return isExpanded_; }
    void setExpanded(bool expanded, bool animated = true);
    void toggle() { setExpanded(!isExpanded_); }

    template <typename T>
    T* setContent(std::unique_ptr<T> content) {
        T* raw = content.get();
        setContentInternal(std::move(content));
        return raw;
    }

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
    [[nodiscard]] const layout::RectLength& contentPadding() const noexcept { return contentPadding_; }

    [[nodiscard]] bool clipsChildren() const noexcept override { return true; }

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    core::Signal<bool> onToggled;

private:
    void setContentInternal(std::unique_ptr<Widget> content);
    void setHeaderAccessoryInternal(std::unique_ptr<Widget> accessory);
    void applySizeMetrics() noexcept;
    float calculateTargetHeight();

    std::string title_{};
    std::shared_ptr<text::Font> font_{};
    SectionHeaderSize headerSize_{SectionHeaderSize::Standard};
    IndicatorPlacement indicatorPlacement_{IndicatorPlacement::Leading};

    mutable std::optional<text::TextLayout> cachedTitleLayout_{};
    mutable bool isTitleLayoutDirty_{true};

    bool isExpanded_{true};
    bool isHoveredHeader_{false};
    bool isPressedHeader_{false};

    float targetBodyHeight_{0.0f};
    core::animation::PropertyAnimation<float> expandAnim_{1.0f};

    layout::RectLength contentPadding_{
        .left = layout::Length::points(8.0f),
        .top = layout::Length::points(4.0f),
        .right = layout::Length::points(8.0f),
        .bottom = layout::Length::points(4.0f),
    };

    Container* body_{nullptr};
    Widget* content_{nullptr};
    Widget* headerAccessory_{nullptr};
};

class Accordion : public Widget {
public:
    explicit Accordion(bool singleExpanded = false,
                       SectionHeaderSize headerSize = SectionHeaderSize::Standard);
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
    [[nodiscard]] std::optional<size_t> indexOfSection(const CollapsibleSection* section) const noexcept;

    [[nodiscard]] bool isSingleExpanded() const noexcept { return singleExpanded_; }
    void setSingleExpanded(bool singleExpanded);

    [[nodiscard]] SectionHeaderSize headerSize() const noexcept { return headerSize_; }
    void setHeaderSize(SectionHeaderSize size) noexcept;

    void focusNextSection(CollapsibleSection* current);
    void focusPrevSection(CollapsibleSection* current);
    void focusFirstSection();
    void focusLastSection();

    core::Signal<size_t, bool> onSectionToggled;

private:
    bool singleExpanded_{false};
    SectionHeaderSize headerSize_{SectionHeaderSize::Standard};
    std::vector<CollapsibleSection*> sections_{};
};

} // namespace northern::widgets
