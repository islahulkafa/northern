#pragma once

#include "core/animation/property_animation.hpp"
#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/widget.hpp"

#include <memory>
#include <string>
#include <vector>

namespace northern::widgets {

class Container;

class CollapsibleSection : public Widget {
public:
    static constexpr float kHeaderHeight = 32.0f;

    explicit CollapsibleSection(std::string title = {}, bool expanded = true,
                                std::shared_ptr<text::Font> font = nullptr);
    ~CollapsibleSection() override = default;

    CollapsibleSection(CollapsibleSection&&) noexcept = default;
    CollapsibleSection& operator=(CollapsibleSection&&) noexcept = default;

    [[nodiscard]] const std::string& title() const noexcept { return title_; }
    void setTitle(std::string title);

    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    void setFont(std::shared_ptr<text::Font> font);

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
    float calculateTargetHeight();

    std::string title_{};
    std::shared_ptr<text::Font> font_{};
    bool isExpanded_{true};
    bool isHoveredHeader_{false};
    bool isPressedHeader_{false};

    float targetBodyHeight_{0.0f};
    core::animation::PropertyAnimation<float> expandAnim_{1.0f};

    Container* body_{nullptr};
    Widget* content_{nullptr};
};

class Accordion : public Widget {
public:
    explicit Accordion(bool singleExpanded = false);
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

    [[nodiscard]] bool isSingleExpanded() const noexcept { return singleExpanded_; }
    void setSingleExpanded(bool singleExpanded);

    core::Signal<size_t, bool> onSectionToggled;

private:
    bool singleExpanded_{false};
    std::vector<CollapsibleSection*> sections_{};
};

} // namespace northern::widgets
