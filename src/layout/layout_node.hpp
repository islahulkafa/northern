#pragma once

#include "core/math/geometry.hpp"
#include "layout/style.hpp"

#include <functional>
#include <memory>
#include <span>
#include <vector>

namespace northern::layout {

class LayoutNode {
public:
    using MeasureFunc = std::function<core::Size(const LayoutNode& node, float availableWidth,
                                                 float availableHeight)>;

    LayoutNode() noexcept = default;
    explicit LayoutNode(Style style) noexcept;
    ~LayoutNode();

    LayoutNode(const LayoutNode&) = delete;
    LayoutNode& operator=(const LayoutNode&) = delete;
    LayoutNode(LayoutNode&&) noexcept;
    LayoutNode& operator=(LayoutNode&&) noexcept;

    // Hierarchy management
    LayoutNode* addChild(std::unique_ptr<LayoutNode> child);
    LayoutNode* addChild(LayoutNode* child);
    std::unique_ptr<LayoutNode> removeChild(LayoutNode* child);
    void detachChild(LayoutNode* child);
    void clearChildren();

    [[nodiscard]] std::span<LayoutNode* const> children() const noexcept { return children_; }
    [[nodiscard]] size_t childCount() const noexcept { return children_.size(); }
    [[nodiscard]] LayoutNode* childAt(size_t index) const noexcept;
    [[nodiscard]] LayoutNode* parent() const noexcept { return parent_; }

    // Style
    [[nodiscard]] const Style& style() const noexcept { return style_; }
    [[nodiscard]] Style& style() noexcept { return style_; }
    void setStyle(const Style& style) noexcept;

    // Intrinsic sizing callback
    void setMeasureFunc(MeasureFunc func);
    [[nodiscard]] bool hasMeasureFunc() const noexcept { return static_cast<bool>(measureFunc_); }
    [[nodiscard]] core::Size measure(float availableWidth, float availableHeight) const;

    // Dirty tracking
    void markDirty() noexcept;
    [[nodiscard]] bool isDirty() const noexcept { return isDirty_; }

    // Layout computation entry point
    void computeLayout(float availableWidth, float availableHeight);

    // Layout output geometry
    [[nodiscard]] core::Rect layoutRect() const noexcept { return layoutRect_; }
    [[nodiscard]] core::Rect contentRect() const noexcept;
    void setLayoutRect(const core::Rect& rect) noexcept { layoutRect_ = rect; }

    // User data attachment
    [[nodiscard]] void* userData() const noexcept { return userData_; }
    void setUserData(void* data) noexcept { userData_ = data; }

private:
    Style style_{};
    LayoutNode* parent_{nullptr};
    std::vector<std::unique_ptr<LayoutNode>> ownedChildren_;
    std::vector<LayoutNode*> children_;
    MeasureFunc measureFunc_{};
    core::Rect layoutRect_{};
    void* userData_{nullptr};
    bool isDirty_{true};
};

} // namespace northern::layout
