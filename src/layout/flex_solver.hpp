#pragma once

#include "layout/layout_node.hpp"

namespace northern::layout {

class FlexSolver {
public:
    static void solve(LayoutNode& root, float availableWidth, float availableHeight);

private:
    struct FlexItem {
        LayoutNode* node{nullptr};
        float baseMain{0.0f};
        float finalMain{0.0f};
        float finalCross{0.0f};
        float mainMarginStart{0.0f};
        float mainMarginEnd{0.0f};
        float crossMarginStart{0.0f};
        float crossMarginEnd{0.0f};
        float minMain{0.0f};
        float maxMain{1e9f};
        float minCross{0.0f};
        float maxCross{1e9f};
        float mainPos{0.0f};
        float crossPos{0.0f};
        bool frozen{false};

        [[nodiscard]] float outerBaseMain() const noexcept {
            return baseMain + mainMarginStart + mainMarginEnd;
        }
        [[nodiscard]] float outerFinalMain() const noexcept {
            return finalMain + mainMarginStart + mainMarginEnd;
        }
        [[nodiscard]] float outerFinalCross() const noexcept {
            return finalCross + crossMarginStart + crossMarginEnd;
        }
    };

    struct FlexLine {
        std::vector<FlexItem> items;
        float crossSize{0.0f};
        float crossPos{0.0f};
    };

    static void solveNode(LayoutNode& node, float availableWidth, float availableHeight);
    static void solveAbsoluteChild(LayoutNode& parent, LayoutNode& child, float parentW,
                                   float parentH, float padLeft, float padTop, float innerW,
                                   float innerH);
};

} // namespace northern::layout
