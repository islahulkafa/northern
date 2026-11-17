#include "core/math/geometry.hpp"

namespace northern::core {

// Explicit template instantiations to ensure coverage
template struct Point2D<float>;
template struct Size2D<float>;
template struct Rect2D<float>;
template struct Insets2D<float>;

template struct Point2D<int32_t>;
template struct Size2D<int32_t>;
template struct Rect2D<int32_t>;
template struct Insets2D<int32_t>;

} // namespace northern::core
