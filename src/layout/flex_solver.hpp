#pragma once

#include "layout/layout_node.hpp"

namespace northern::core {
class Arena;
}

namespace northern::layout {

class FlexSolver {
public:
    static void solve(LayoutNode& root, float availableWidth, float availableHeight);

private:
    static void solveNode(LayoutNode& node, float availableWidth, float availableHeight,
                          core::Arena& arena);
    static void solveAbsoluteChild(LayoutNode& parent, LayoutNode& child, float parentW,
                                   float parentH, float padLeft, float padTop, float innerW,
                                   float innerH, core::Arena& arena);
};

} // namespace northern::layout
