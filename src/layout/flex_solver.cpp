#include "layout/flex_solver.hpp"
#include "core/memory/arena.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace northern::layout {

namespace {

thread_local core::Arena s_scratchArena{64 * 1024};
thread_local int s_solverDepth{0};

struct DepthGuard {
    int& depth;
    explicit DepthGuard(int& d) noexcept : depth(d) { ++depth; }
    ~DepthGuard() noexcept { --depth; }
};

float clampDimension(float val, float minVal, float maxVal) {
    if (maxVal < minVal) {
        maxVal = minVal;
    }
    return std::clamp(val, minVal, maxVal);
}

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

using ItemAllocator = core::ArenaAllocator<FlexItem>;
using FlexItemVector = std::vector<FlexItem, ItemAllocator>;

struct FlexLine {
    FlexItemVector items;
    float crossSize{0.0f};
    float crossPos{0.0f};

    explicit FlexLine(core::Arena& arena) : items(ItemAllocator(arena)) {}
};

using LineAllocator = core::ArenaAllocator<FlexLine>;
using FlexLineVector = std::vector<FlexLine, LineAllocator>;

using NodePtrAllocator = core::ArenaAllocator<LayoutNode*>;
using NodePtrVector = std::vector<LayoutNode*, NodePtrAllocator>;

} // namespace

void FlexSolver::solve(LayoutNode& root, float availableWidth, float availableHeight) {
    if (s_solverDepth == 0) {
        core::ScopedArena scoped(s_scratchArena);
        DepthGuard guard(s_solverDepth);
        solveNode(root, availableWidth, availableHeight, s_scratchArena);
    } else {
        DepthGuard guard(s_solverDepth);
        solveNode(root, availableWidth, availableHeight, s_scratchArena);
    }
}

void FlexSolver::solveNode(LayoutNode& node, float availableWidth, float availableHeight,
                           core::Arena& arena) {
    const Style& style = node.style();
    if (style.display == Display::None) {
        node.setLayoutRect(core::Rect{0.0f, 0.0f, 0.0f, 0.0f});
        return;
    }

    // Determine container size
    float parentW = (availableWidth > 0.0f) ? availableWidth : 0.0f;
    float parentH = (availableHeight > 0.0f) ? availableHeight : 0.0f;

    float minW = style.minWidth.resolve(parentW, 0.0f);
    float maxW = style.maxWidth.resolve(parentW, 1e9f);
    float minH = style.minHeight.resolve(parentH, 0.0f);
    float maxH = style.maxHeight.resolve(parentH, 1e9f);

    float containerW = 0.0f;
    bool fixedW = false;
    float containerH = 0.0f;
    bool fixedH = false;

    if (node.parent()) {
        if (availableWidth > 0.0f) {
            containerW = clampDimension(availableWidth, minW, maxW);
            fixedW = true;
        } else if (style.width.isPoints() || style.width.isPercent()) {
            containerW = clampDimension(style.width.resolve(parentW), minW, maxW);
            fixedW = true;
        }

        if (availableHeight > 0.0f) {
            containerH = clampDimension(availableHeight, minH, maxH);
            fixedH = true;
        } else if (style.height.isPoints() || style.height.isPercent()) {
            containerH = clampDimension(style.height.resolve(parentH), minH, maxH);
            fixedH = true;
        }
    } else {
        if (style.width.isPoints() || style.width.isPercent()) {
            containerW = clampDimension(style.width.resolve(parentW), minW, maxW);
            fixedW = true;
        } else if (!node.hasMeasureFunc() && availableWidth > 0.0f) {
            containerW = clampDimension(availableWidth, minW, maxW);
            fixedW = true;
        }

        if (style.height.isPoints() || style.height.isPercent()) {
            containerH = clampDimension(style.height.resolve(parentH), minH, maxH);
            fixedH = true;
        } else if (!node.hasMeasureFunc() && availableHeight > 0.0f) {
            containerH = clampDimension(availableHeight, minH, maxH);
            fixedH = true;
        }
    }

    if (style.aspectRatio.has_value() && *style.aspectRatio > 0.0f) {
        float ratio = *style.aspectRatio;
        if (fixedW && !fixedH) {
            containerH = clampDimension(containerW / ratio, minH, maxH);
            fixedH = true;
        } else if (!fixedW && fixedH) {
            containerW = clampDimension(containerH * ratio, minW, maxW);
            fixedW = true;
        }
    }

    // Check if leaf node with measure callback
    if (node.hasMeasureFunc() && node.childCount() == 0) {
        core::Size measured = node.measure(fixedW ? containerW : availableWidth,
                                           fixedH ? containerH : availableHeight);
        if (!fixedW) {
            containerW = clampDimension(measured.width, minW, maxW);
        }
        if (!fixedH) {
            containerH = clampDimension(measured.height, minH, maxH);
        }
        if (style.aspectRatio.has_value() && *style.aspectRatio > 0.0f) {
            float ratio = *style.aspectRatio;
            if (fixedW && !fixedH) {
                containerH = clampDimension(containerW / ratio, minH, maxH);
            } else if (!fixedW && fixedH) {
                containerW = clampDimension(containerH * ratio, minW, maxW);
            }
        }
        node.setLayoutRect(core::Rect{node.layoutRect().origin, {containerW, containerH}});
        return;
    }

    // Padding
    float padLeft = style.padding.left.resolve(fixedW ? containerW : parentW, 0.0f);
    float padTop = style.padding.top.resolve(fixedH ? containerH : parentH, 0.0f);
    float padRight = style.padding.right.resolve(fixedW ? containerW : parentW, 0.0f);
    float padBottom = style.padding.bottom.resolve(fixedH ? containerH : parentH, 0.0f);

    float innerW = fixedW ? std::max(0.0f, containerW - padLeft - padRight) : 1e9f;
    float innerH = fixedH ? std::max(0.0f, containerH - padTop - padBottom) : 1e9f;

    bool isRow = (style.flexDirection == FlexDirection::Row ||
                  style.flexDirection == FlexDirection::RowReverse);
    bool isReverse = (style.flexDirection == FlexDirection::RowReverse ||
                      style.flexDirection == FlexDirection::ColumnReverse);

    float mainInner = isRow ? innerW : innerH;
    float crossInner = isRow ? innerH : innerW;
    float mainGap = isRow ? style.columnGap : style.rowGap;
    float crossGap = isRow ? style.rowGap : style.columnGap;

    // Collect in-flow and out-of-flow children
    FlexItemVector items{ItemAllocator(arena)};
    NodePtrVector absoluteChildren{NodePtrAllocator(arena)};

    for (LayoutNode* child : node.children()) {
        if (child->style().display == Display::None) {
            child->setLayoutRect(core::Rect{0.0f, 0.0f, 0.0f, 0.0f});
            continue;
        }
        if (child->style().positionType == PositionType::Absolute) {
            absoluteChildren.push_back(child);
            continue;
        }

        const Style& cs = child->style();
        FlexItem item{};
        item.node = child;

        // Margins
        float mLeft = cs.margin.left.resolve(fixedW ? innerW : parentW, 0.0f);
        float mTop = cs.margin.top.resolve(fixedH ? innerH : parentH, 0.0f);
        float mRight = cs.margin.right.resolve(fixedW ? innerW : parentW, 0.0f);
        float mBottom = cs.margin.bottom.resolve(fixedH ? innerH : parentH, 0.0f);

        item.mainMarginStart = isRow ? mLeft : mTop;
        item.mainMarginEnd = isRow ? mRight : mBottom;
        item.crossMarginStart = isRow ? mTop : mLeft;
        item.crossMarginEnd = isRow ? mBottom : mRight;

        // Constraints
        float cMinW = cs.minWidth.resolve(fixedW ? innerW : parentW, 0.0f);
        float cMaxW = cs.maxWidth.resolve(fixedW ? innerW : parentW, 1e9f);
        float cMinH = cs.minHeight.resolve(fixedH ? innerH : parentH, 0.0f);
        float cMaxH = cs.maxHeight.resolve(fixedH ? innerH : parentH, 1e9f);

        item.minMain = isRow ? cMinW : cMinH;
        item.maxMain = isRow ? cMaxW : cMaxH;
        item.minCross = isRow ? cMinH : cMinW;
        item.maxCross = isRow ? cMaxH : cMaxW;

        float measureAvailW = isRow ? (cMaxW < 1e8f ? cMaxW : 0.0f)
                                    : clampDimension((cs.width.isPoints() || cs.width.isPercent())
                                                         ? cs.width.resolve(crossInner)
                                                         : crossInner,
                                                     cMinW, cMaxW);
        float measureAvailH = isRow ? clampDimension((cs.height.isPoints() || cs.height.isPercent())
                                                         ? cs.height.resolve(crossInner)
                                                         : crossInner,
                                                     cMinH, cMaxH)
                                    : (cMaxH < 1e8f ? cMaxH : 0.0f);

        // Base main size
        if (cs.flexBasis.isPoints() || cs.flexBasis.isPercent()) {
            item.baseMain = cs.flexBasis.resolve(mainInner);
        } else if ((isRow && (cs.width.isPoints() || cs.width.isPercent())) ||
                   (!isRow && (cs.height.isPoints() || cs.height.isPercent()))) {
            item.baseMain = isRow ? cs.width.resolve(mainInner) : cs.height.resolve(mainInner);
        } else if (cs.aspectRatio.has_value() && *cs.aspectRatio > 0.0f) {
            float ratio = *cs.aspectRatio;
            if (isRow && (cs.height.isPoints() || cs.height.isPercent())) {
                float definiteCross = cs.height.resolve(crossInner);
                item.baseMain = definiteCross * ratio;
                item.finalCross = definiteCross;
            } else if (!isRow && (cs.width.isPoints() || cs.width.isPercent())) {
                float definiteCross = cs.width.resolve(crossInner);
                item.baseMain = definiteCross / ratio;
                item.finalCross = definiteCross;
            } else if (child->hasMeasureFunc()) {
                core::Size m = child->measure(measureAvailW, measureAvailH);
                item.baseMain = isRow ? m.width : m.height;
                item.finalCross = isRow ? m.height : m.width;
            } else if (child->childCount() > 0) {
                AlignSelf childAlign = (cs.alignSelf != AlignSelf::Auto)
                                           ? cs.alignSelf
                                           : toAlignSelf(style.alignItems);
                bool stretchCross = (childAlign == AlignSelf::Stretch) &&
                                    !(isRow ? (cs.height.isPoints() || cs.height.isPercent())
                                            : (cs.width.isPoints() || cs.width.isPercent()));
                float childAvailW =
                    isRow ? 0.0f
                          : (stretchCross ? crossInner
                                          : ((cs.width.isPoints() || cs.width.isPercent())
                                                 ? cs.width.resolve(crossInner)
                                                 : 0.0f));
                float childAvailH =
                    isRow ? (stretchCross ? crossInner
                                          : ((cs.height.isPoints() || cs.height.isPercent())
                                                 ? cs.height.resolve(crossInner)
                                                 : 0.0f))
                          : 0.0f;
                solveNode(*child, childAvailW, childAvailH, arena);
                item.baseMain =
                    isRow ? child->layoutRect().size.width : child->layoutRect().size.height;
                item.finalCross =
                    isRow ? child->layoutRect().size.height : child->layoutRect().size.width;
            }
        } else if (child->hasMeasureFunc()) {
            core::Size m = child->measure(measureAvailW, measureAvailH);
            item.baseMain = isRow ? m.width : m.height;
            item.finalCross = isRow ? m.height : m.width;
        } else if (child->childCount() > 0) {
            AlignSelf childAlign =
                (cs.alignSelf != AlignSelf::Auto) ? cs.alignSelf : toAlignSelf(style.alignItems);
            bool stretchCross = (childAlign == AlignSelf::Stretch) &&
                                !(isRow ? (cs.height.isPoints() || cs.height.isPercent())
                                        : (cs.width.isPoints() || cs.width.isPercent()));
            float childAvailW = isRow
                                    ? 0.0f
                                    : (stretchCross ? crossInner
                                                    : ((cs.width.isPoints() || cs.width.isPercent())
                                                           ? cs.width.resolve(crossInner)
                                                           : 0.0f));
            float childAvailH =
                isRow ? (stretchCross ? crossInner
                                      : ((cs.height.isPoints() || cs.height.isPercent())
                                             ? cs.height.resolve(crossInner)
                                             : 0.0f))
                      : 0.0f;
            solveNode(*child, childAvailW, childAvailH, arena);
            item.baseMain =
                isRow ? child->layoutRect().size.width : child->layoutRect().size.height;
            item.finalCross =
                isRow ? child->layoutRect().size.height : child->layoutRect().size.width;
        }

        item.baseMain = clampDimension(item.baseMain, item.minMain, item.maxMain);
        item.finalMain = item.baseMain;

        // Initial cross size
        if (isRow) {
            if (cs.height.isPoints() || cs.height.isPercent()) {
                item.finalCross = cs.height.resolve(crossInner);
            } else if (cs.aspectRatio.has_value() && *cs.aspectRatio > 0.0f) {
                item.finalCross = item.baseMain / (*cs.aspectRatio);
            }
        } else {
            if (cs.width.isPoints() || cs.width.isPercent()) {
                item.finalCross = cs.width.resolve(crossInner);
            } else if (cs.aspectRatio.has_value() && *cs.aspectRatio > 0.0f) {
                item.finalCross = item.baseMain * (*cs.aspectRatio);
            }
        }
        item.finalCross = clampDimension(item.finalCross, item.minCross, item.maxCross);

        items.push_back(item);
    }

    // Break into flex lines (Wrap)
    FlexLineVector lines{LineAllocator(arena)};
    if (items.empty()) {
        lines.emplace_back(arena);
    } else if (style.flexWrap == FlexWrap::NoWrap) {
        FlexLine line{arena};
        line.items = std::move(items);
        lines.push_back(std::move(line));
    } else {
        FlexLine currentLine{arena};
        float currentLineMain = 0.0f;

        for (auto& item : items) {
            float itemOuter = item.outerBaseMain();
            float spaceWithItem =
                currentLine.items.empty() ? itemOuter : (currentLineMain + mainGap + itemOuter);

            if (!currentLine.items.empty() && mainInner < 1e8f && spaceWithItem > mainInner) {
                lines.push_back(std::move(currentLine));
                currentLine = FlexLine{arena};
                currentLine.items.push_back(item);
                currentLineMain = itemOuter;
            } else {
                currentLine.items.push_back(item);
                currentLineMain = spaceWithItem;
            }
        }
        if (!currentLine.items.empty()) {
            lines.push_back(std::move(currentLine));
        }
    }

    // 3. Resolve flexible lengths per line (Grow / Shrink)
    for (auto& line : lines) {
        if (line.items.empty()) {
            continue;
        }

        float totalBaseMain = 0.0f;
        for (const auto& item : line.items) {
            totalBaseMain += item.outerBaseMain();
        }
        totalBaseMain += mainGap * std::max(0.0f, static_cast<float>(line.items.size() - 1));

        float freeSpace = (mainInner < 1e8f) ? (mainInner - totalBaseMain) : 0.0f;

        if (freeSpace > 0.0f) {
            // Flex Grow loop with clamping
            bool hasUnfrozen = true;
            while (hasUnfrozen) {
                float totalGrow = 0.0f;
                for (const auto& itm : line.items) {
                    if (!itm.frozen) {
                        totalGrow += itm.node->style().flexGrow;
                    }
                }
                if (totalGrow <= 0.0f) {
                    break;
                }

                hasUnfrozen = false;
                float spaceToDistribute = freeSpace;
                for (auto& itm : line.items) {
                    if (itm.frozen) {
                        continue;
                    }
                    float grow = itm.node->style().flexGrow;
                    if (grow > 0.0f) {
                        float added = (spaceToDistribute * grow) / totalGrow;
                        float newMain = itm.baseMain + added;
                        if (newMain >= itm.maxMain) {
                            itm.finalMain = itm.maxMain;
                            itm.frozen = true;
                            freeSpace -= (itm.maxMain - itm.baseMain);
                            hasUnfrozen = true;
                            break;
                        } else {
                            itm.finalMain = newMain;
                        }
                    }
                }
            }
        } else if (freeSpace < 0.0f) {
            // Flex Shrink loop with clamping
            float deficit = -freeSpace;
            bool hasUnfrozen = true;
            while (hasUnfrozen) {
                float totalScaledShrink = 0.0f;
                for (const auto& itm : line.items) {
                    if (!itm.frozen) {
                        totalScaledShrink += itm.node->style().flexShrink * itm.baseMain;
                    }
                }
                if (totalScaledShrink <= 0.0f) {
                    break;
                }

                hasUnfrozen = false;
                float deficitToDistribute = deficit;
                for (auto& itm : line.items) {
                    if (itm.frozen) {
                        continue;
                    }
                    float shrink = itm.node->style().flexShrink;
                    if (shrink > 0.0f) {
                        float removed =
                            (deficitToDistribute * (shrink * itm.baseMain)) / totalScaledShrink;
                        float newMain = itm.baseMain - removed;
                        if (newMain <= itm.minMain) {
                            itm.finalMain = itm.minMain;
                            itm.frozen = true;
                            deficit -= (itm.baseMain - itm.minMain);
                            hasUnfrozen = true;
                            break;
                        } else {
                            itm.finalMain = newMain;
                        }
                    }
                }
            }
        }

        // Measure children again if main size changed and they have measure callbacks or aspect
        // ratio
        for (auto& itm : line.items) {
            const Style& cs = itm.node->style();
            if (cs.aspectRatio.has_value() && *cs.aspectRatio > 0.0f) {
                float ratio = *cs.aspectRatio;
                if (isRow && !cs.height.isPoints() && !cs.height.isPercent()) {
                    itm.finalCross =
                        clampDimension(itm.finalMain / ratio, itm.minCross, itm.maxCross);
                } else if (!isRow && !cs.width.isPoints() && !cs.width.isPercent()) {
                    itm.finalCross =
                        clampDimension(itm.finalMain * ratio, itm.minCross, itm.maxCross);
                }
            } else if (itm.node->hasMeasureFunc()) {
                core::Size m = itm.node->measure(isRow ? itm.finalMain : crossInner,
                                                 isRow ? crossInner : itm.finalMain);
                if (isRow && !itm.node->style().height.isPoints() &&
                    !itm.node->style().height.isPercent()) {
                    itm.finalCross = clampDimension(m.height, itm.minCross, itm.maxCross);
                } else if (!isRow && !itm.node->style().width.isPoints() &&
                           !itm.node->style().width.isPercent()) {
                    itm.finalCross = clampDimension(m.width, itm.minCross, itm.maxCross);
                }
            }

            // Cross size of line is max outer cross of items
            line.crossSize = std::max(line.crossSize, itm.outerFinalCross());
        }
    }

    // 4. Compute unconstrained container cross size if Auto
    float totalLinesCrossSize = 0.0f;
    for (size_t i = 0; i < lines.size(); ++i) {
        totalLinesCrossSize += lines[i].crossSize;
        if (i + 1 < lines.size()) {
            totalLinesCrossSize += crossGap;
        }
    }

    float maxLineMain = 0.0f;
    for (const auto& line : lines) {
        float lineMain = 0.0f;
        for (const auto& itm : line.items) {
            lineMain += itm.outerFinalMain();
        }
        lineMain += mainGap * std::max(0.0f, static_cast<float>(line.items.size() - 1));
        maxLineMain = std::max(maxLineMain, lineMain);
    }

    if (!fixedW) {
        float computedW = isRow ? maxLineMain : totalLinesCrossSize;
        containerW = clampDimension(computedW + padLeft + padRight, minW, maxW);
        innerW = std::max(0.0f, containerW - padLeft - padRight);
        if (isRow) {
            mainInner = innerW;
        } else {
            crossInner = innerW;
        }
    }

    if (!fixedH) {
        float computedH = isRow ? totalLinesCrossSize : maxLineMain;
        containerH = clampDimension(computedH + padTop + padBottom, minH, maxH);
        innerH = std::max(0.0f, containerH - padTop - padBottom);
        if (isRow) {
            crossInner = innerH;
        } else {
            mainInner = innerH;
        }
    }

    if (style.aspectRatio.has_value() && *style.aspectRatio > 0.0f) {
        float ratio = *style.aspectRatio;
        if (fixedW && !fixedH) {
            containerH = clampDimension(containerW / ratio, minH, maxH);
            innerH = std::max(0.0f, containerH - padTop - padBottom);
            if (isRow) {
                crossInner = innerH;
            } else {
                mainInner = innerH;
            }
        } else if (!fixedW && fixedH) {
            containerW = clampDimension(containerH * ratio, minW, maxW);
            innerW = std::max(0.0f, containerW - padLeft - padRight);
            if (isRow) {
                mainInner = innerW;
            } else {
                crossInner = innerW;
            }
        }
    }

    // If single-line and container cross dimension was fixed, line expands to container crossInner
    if (lines.size() == 1 && crossInner > 0.0f && crossInner < 1e8f) {
        lines[0].crossSize = std::max(lines[0].crossSize, crossInner);
    }

    // 5. Position lines along cross axis (alignContent)
    float remainingCross = std::max(0.0f, crossInner - totalLinesCrossSize);
    float currentCross = 0.0f;

    if (style.alignContent == AlignContent::FlexEnd) {
        currentCross = remainingCross;
    } else if (style.alignContent == AlignContent::Center) {
        currentCross = remainingCross * 0.5f;
    }

    for (size_t i = 0; i < lines.size(); ++i) {
        lines[i].crossPos = currentCross;
        currentCross += lines[i].crossSize + crossGap;
    }

    // 6. Position items along main axis and cross axis
    for (auto& line : lines) {
        if (line.items.empty()) {
            continue;
        }

        float lineUsedMain = 0.0f;
        for (const auto& itm : line.items) {
            lineUsedMain += itm.outerFinalMain();
        }
        lineUsedMain += mainGap * std::max(0.0f, static_cast<float>(line.items.size() - 1));

        float remainingMain = std::max(0.0f, mainInner - lineUsedMain);
        float currentMain = 0.0f;
        float itemGap = mainGap;

        switch (style.justifyContent) {
        case JustifyContent::FlexEnd:
            currentMain = remainingMain;
            break;
        case JustifyContent::Center:
            currentMain = remainingMain * 0.5f;
            break;
        case JustifyContent::SpaceBetween:
            if (line.items.size() > 1) {
                itemGap = mainGap + (remainingMain / static_cast<float>(line.items.size() - 1));
            }
            break;
        case JustifyContent::SpaceAround:
            if (!line.items.empty()) {
                float spacePerItem = remainingMain / static_cast<float>(line.items.size());
                currentMain = spacePerItem * 0.5f;
                itemGap = mainGap + spacePerItem;
            }
            break;
        case JustifyContent::SpaceEvenly:
            if (!line.items.empty()) {
                float spaceBetween = remainingMain / static_cast<float>(line.items.size() + 1);
                currentMain = spaceBetween;
                itemGap = mainGap + spaceBetween;
            }
            break;
        case JustifyContent::FlexStart:
        default:
            currentMain = 0.0f;
            break;
        }

        size_t count = line.items.size();
        for (size_t idx = 0; idx < count; ++idx) {
            size_t itemIdx = isReverse ? (count - 1 - idx) : idx;
            auto& itm = line.items[itemIdx];

            itm.mainPos = currentMain + itm.mainMarginStart;
            currentMain += itm.outerFinalMain() + itemGap;

            // Cross axis alignment
            AlignSelf align = (itm.node->style().alignSelf != AlignSelf::Auto)
                                  ? itm.node->style().alignSelf
                                  : toAlignSelf(style.alignItems);

            if (align == AlignSelf::Stretch) {
                // If cross dimension not explicitly set and no aspect ratio, stretch to line cross
                // size
                bool hasExplicitCross = isRow ? (itm.node->style().height.isPoints() ||
                                                 itm.node->style().height.isPercent())
                                              : (itm.node->style().width.isPoints() ||
                                                 itm.node->style().width.isPercent());
                bool hasAspectRatio = itm.node->style().aspectRatio.has_value() &&
                                      *itm.node->style().aspectRatio > 0.0f;
                if (!hasExplicitCross && !hasAspectRatio) {
                    float stretched = line.crossSize - itm.crossMarginStart - itm.crossMarginEnd;
                    itm.finalCross = clampDimension(stretched, itm.minCross, itm.maxCross);
                }
            }

            float availableLineCross = line.crossSize - itm.outerFinalCross();
            float crossOffset = 0.0f;

            switch (align) {
            case AlignSelf::FlexEnd:
                crossOffset = availableLineCross;
                break;
            case AlignSelf::Center:
                crossOffset = availableLineCross * 0.5f;
                break;
            case AlignSelf::FlexStart:
            case AlignSelf::Stretch:
            default:
                crossOffset = 0.0f;
                break;
            }

            itm.crossPos = line.crossPos + itm.crossMarginStart + crossOffset;

            // Compute final geometry
            float x = isRow ? itm.mainPos : itm.crossPos;
            float y = isRow ? itm.crossPos : itm.mainPos;
            float w = isRow ? itm.finalMain : itm.finalCross;
            float h = isRow ? itm.finalCross : itm.finalMain;

            // Position relative to parent origin (accounting for container padding)
            core::Rect childRect{{padLeft + x, padTop + y}, {w, h}};
            itm.node->setLayoutRect(childRect);

            // Recursively solve children of item
            solveNode(*itm.node, w, h, arena);
        }
    }

    // 7. Position out-of-flow (Absolute) children
    for (auto* absChild : absoluteChildren) {
        solveAbsoluteChild(node, *absChild, containerW, containerH, padLeft, padTop, innerW, innerH,
                           arena);
    }

    node.setLayoutRect(core::Rect{node.layoutRect().origin, {containerW, containerH}});
}

void FlexSolver::solveAbsoluteChild(LayoutNode& parent, LayoutNode& child, float parentW,
                                    float parentH, float padLeft, float padTop, float innerW,
                                    float innerH, core::Arena& arena) {
    (void)parent;
    const Style& cs = child.style();

    float minW = cs.minWidth.resolve(innerW, 0.0f);
    float maxW = cs.maxWidth.resolve(innerW, 1e9f);
    float minH = cs.minHeight.resolve(innerH, 0.0f);
    float maxH = cs.maxHeight.resolve(innerH, 1e9f);

    float childW = 0.0f;
    bool fixedChildW = false;
    if (cs.width.isPoints() || cs.width.isPercent()) {
        childW = clampDimension(cs.width.resolve(innerW), minW, maxW);
        fixedChildW = true;
    } else if (child.hasMeasureFunc()) {
        childW = clampDimension(child.measure(innerW, innerH).width, minW, maxW);
        fixedChildW = true;
    }

    float childH = 0.0f;
    bool fixedChildH = false;
    if (cs.height.isPoints() || cs.height.isPercent()) {
        childH = clampDimension(cs.height.resolve(innerH), minH, maxH);
        fixedChildH = true;
    } else if (child.hasMeasureFunc()) {
        childH = clampDimension(child.measure(innerW, innerH).height, minH, maxH);
        fixedChildH = true;
    }

    if (cs.aspectRatio.has_value() && *cs.aspectRatio > 0.0f) {
        float ratio = *cs.aspectRatio;
        if (fixedChildW && !fixedChildH) {
            childH = clampDimension(childW / ratio, minH, maxH);
        } else if (!fixedChildW && fixedChildH) {
            childW = clampDimension(childH * ratio, minW, maxW);
        }
    }

    float x = padLeft;
    if (!cs.position.left.isAuto()) {
        x = padLeft + cs.position.left.resolve(innerW, 0.0f);
    } else if (!cs.position.right.isAuto()) {
        x = parentW - cs.position.right.resolve(innerW, 0.0f) - childW;
    }

    float y = padTop;
    if (!cs.position.top.isAuto()) {
        y = padTop + cs.position.top.resolve(innerH, 0.0f);
    } else if (!cs.position.bottom.isAuto()) {
        y = parentH - cs.position.bottom.resolve(innerH, 0.0f) - childH;
    }

    child.setLayoutRect(core::Rect{{x, y}, {childW, childH}});
    solveNode(child, childW, childH, arena);
}

} // namespace northern::layout
