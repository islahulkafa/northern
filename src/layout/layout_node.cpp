#include "layout/layout_node.hpp"
#include "layout/flex_solver.hpp"

#include <algorithm>

namespace northern::layout {

LayoutNode::LayoutNode(Style style) noexcept : style_(style) {}

LayoutNode::~LayoutNode() = default;

LayoutNode::LayoutNode(LayoutNode&& other) noexcept
    : style_(other.style_), parent_(other.parent_), ownedChildren_(std::move(other.ownedChildren_)),
      children_(std::move(other.children_)), measureFunc_(std::move(other.measureFunc_)),
      layoutRect_(other.layoutRect_), userData_(other.userData_), isDirty_(other.isDirty_) {
    other.parent_ = nullptr;
    other.userData_ = nullptr;
    for (auto* child : children_) {
        if (child) {
            child->parent_ = this;
        }
    }
}

LayoutNode& LayoutNode::operator=(LayoutNode&& other) noexcept {
    if (this != &other) {
        style_ = other.style_;
        parent_ = other.parent_;
        ownedChildren_ = std::move(other.ownedChildren_);
        children_ = std::move(other.children_);
        measureFunc_ = std::move(other.measureFunc_);
        layoutRect_ = other.layoutRect_;
        userData_ = other.userData_;
        isDirty_ = other.isDirty_;

        other.parent_ = nullptr;
        other.userData_ = nullptr;
        for (auto* child : children_) {
            if (child) {
                child->parent_ = this;
            }
        }
    }
    return *this;
}

LayoutNode* LayoutNode::addChild(std::unique_ptr<LayoutNode> child) {
    if (!child) {
        return nullptr;
    }
    child->parent_ = this;
    LayoutNode* ptr = child.get();
    ownedChildren_.push_back(std::move(child));
    children_.push_back(ptr);
    markDirty();
    return ptr;
}

LayoutNode* LayoutNode::addChild(LayoutNode* child) {
    if (!child) {
        return nullptr;
    }
    child->parent_ = this;
    children_.push_back(child);
    markDirty();
    return child;
}

std::unique_ptr<LayoutNode> LayoutNode::removeChild(LayoutNode* child) {
    if (!child) {
        return nullptr;
    }
    auto itChild = std::find(children_.begin(), children_.end(), child);
    if (itChild != children_.end()) {
        children_.erase(itChild);
        child->parent_ = nullptr;
        markDirty();
    }
    auto itOwned = std::find_if(ownedChildren_.begin(), ownedChildren_.end(),
                                [child](const auto& ptr) { return ptr.get() == child; });
    if (itOwned != ownedChildren_.end()) {
        std::unique_ptr<LayoutNode> removed = std::move(*itOwned);
        ownedChildren_.erase(itOwned);
        return removed;
    }
    return nullptr;
}

void LayoutNode::detachChild(LayoutNode* child) {
    if (!child) {
        return;
    }
    auto itChild = std::find(children_.begin(), children_.end(), child);
    if (itChild != children_.end()) {
        children_.erase(itChild);
        child->parent_ = nullptr;
        markDirty();
    }
    auto itOwned = std::find_if(ownedChildren_.begin(), ownedChildren_.end(),
                                [child](const auto& ptr) { return ptr.get() == child; });
    if (itOwned != ownedChildren_.end()) {
        ownedChildren_.erase(itOwned);
    }
}

void LayoutNode::clearChildren() {
    for (auto* child : children_) {
        if (child) {
            child->parent_ = nullptr;
        }
    }
    ownedChildren_.clear();
    children_.clear();
    markDirty();
}

LayoutNode* LayoutNode::childAt(size_t index) const noexcept {
    if (index < children_.size()) {
        return children_[index];
    }
    return nullptr;
}

void LayoutNode::setStyle(const Style& style) noexcept {
    style_ = style;
    markDirty();
}

void LayoutNode::setMeasureFunc(MeasureFunc func) {
    measureFunc_ = std::move(func);
    markDirty();
}

core::Size LayoutNode::measure(float availableWidth, float availableHeight) const {
    if (measureFunc_) {
        return measureFunc_(*this, availableWidth, availableHeight);
    }
    return core::Size{0.0f, 0.0f};
}

void LayoutNode::markDirty() noexcept {
    isDirty_ = true;
    if (parent_) {
        parent_->markDirty();
    }
}

void LayoutNode::computeLayout(float availableWidth, float availableHeight) {
    FlexSolver::solve(*this, availableWidth, availableHeight);
    isDirty_ = false;
}

core::Rect LayoutNode::contentRect() const noexcept {
    float w = layoutRect_.size.width;
    float h = layoutRect_.size.height;
    float pLeft = style_.padding.left.resolve(w, 0.0f);
    float pTop = style_.padding.top.resolve(h, 0.0f);
    float pRight = style_.padding.right.resolve(w, 0.0f);
    float pBottom = style_.padding.bottom.resolve(h, 0.0f);

    float cw = std::max(0.0f, w - pLeft - pRight);
    float ch = std::max(0.0f, h - pTop - pBottom);
    return core::Rect{{pLeft, pTop}, {cw, ch}};
}

} // namespace northern::layout
