#pragma once

#include "core/animation/property_animation.hpp"
#include "core/math/color.hpp"
#include "core/signal/connection.hpp"
#include "platform/window.hpp"
#include "text/font.hpp"
#include "text/text_layout.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/controls/icon.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace northern::widgets {

class Container;

enum class TitleBarSize : uint8_t {
    Compact,
    Standard,
    Comfortable,
};

class TitleBar : public Widget {
public:
    static constexpr float kCompactHeight = 32.0f;
    static constexpr float kDefaultHeight = 36.0f;
    static constexpr float kComfortableHeight = 40.0f;
    static constexpr float kCaptionButtonWidth = 44.0f;

    explicit TitleBar(std::string title = "Northern", std::shared_ptr<text::Font> font = nullptr,
                      platform::Window* window = nullptr,
                      TitleBarSize size = TitleBarSize::Standard);
    ~TitleBar() override;

    TitleBar(TitleBar&&) noexcept = default;
    TitleBar& operator=(TitleBar&&) noexcept = default;

    void setWindow(platform::Window* window);
    [[nodiscard]] platform::Window* window() const noexcept { return window_; }

    [[nodiscard]] const std::string& title() const noexcept { return title_; }
    void setTitle(std::string title);

    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setSize(TitleBarSize size) noexcept;
    [[nodiscard]] TitleBarSize size() const noexcept { return size_; }

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setShowLogo(bool show) noexcept;
    [[nodiscard]] bool showLogo() const noexcept { return showLogo_; }

    void setMinimizeVisible(bool visible) noexcept;
    [[nodiscard]] bool isMinimizeVisible() const noexcept { return showMinimize_; }

    void setMaximizeVisible(bool visible) noexcept;
    [[nodiscard]] bool isMaximizeVisible() const noexcept { return showMaximize_; }

    void setCloseVisible(bool visible) noexcept;
    [[nodiscard]] bool isCloseVisible() const noexcept { return showClose_; }

    void setBackgroundColor(const core::Color& color) noexcept;
    void setBorderColor(const core::Color& color) noexcept;
    void setTitleColor(const core::Color& color) noexcept;
    void clearColorOverrides() noexcept;

    [[nodiscard]] core::Color effectiveBackgroundColor() const noexcept;
    [[nodiscard]] core::Color effectiveBorderColor() const noexcept;
    [[nodiscard]] core::Color effectiveTitleColor() const noexcept;

    void setCenterWidget(std::unique_ptr<Widget> widget);
    [[nodiscard]] Widget* centerWidget() const noexcept { return centerWidget_; }

    void addActionWidget(std::unique_ptr<Widget> widget);
    void removeActionWidget(Widget* widget);
    void clearActionWidgets();
    [[nodiscard]] const std::vector<Widget*>& actionWidgets() const noexcept {
        return actionWidgets_;
    }

    [[nodiscard]] platform::HitTestResult hitTestPoint(core::Point screenPos) const;

    void updateWindowState();

    void updateAnimations(float deltaSeconds) override;
    [[nodiscard]] bool hasActiveAnimations() const noexcept override;

    void paint(renderer::PaintContext& ctx) override;

    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseMove(WidgetMouseEvent& ev) override;
    void onMouseDown(WidgetMouseEvent& ev) override;
    void onMouseUp(WidgetMouseEvent& ev) override;

    core::Signal<> onMinimizeClicked;
    core::Signal<> onMaximizeClicked;
    core::Signal<> onCloseClicked;

private:
    enum class CaptionButton { None, Minimize, Maximize, Close };

    [[nodiscard]] CaptionButton hitTestCaptionButton(core::Point localPos) const;
    [[nodiscard]] core::Rect captionButtonRect(CaptionButton btn) const;
    [[nodiscard]] int visibleCaptionButtonCount() const noexcept;
    void updateZoneLayout() noexcept;
    void invalidateTitleCache() noexcept;
    void ensureTitleLayout(float maxWidth) const;
    void updateHoverAnimations(CaptionButton prevBtn, CaptionButton nextBtn);
    void updateCaptionTooltip(CaptionButton btn);

    std::string title_;
    std::shared_ptr<text::Font> font_;
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    platform::Window* window_{nullptr};
    core::ScopedConnection windowResizeConn_{};

    TitleBarSize size_{TitleBarSize::Standard};
    float dpiScale_{1.0f};
    bool showLogo_{true};
    bool showMinimize_{true};
    bool showMaximize_{true};
    bool showClose_{true};

    std::unique_ptr<Icon> appLogo_;
    std::unique_ptr<Icon> minIcon_;
    std::unique_ptr<Icon> maxIcon_;
    std::unique_ptr<Icon> restoreIcon_;
    std::unique_ptr<Icon> closeIcon_;

    Container* leftZone_{nullptr};
    Container* centerZone_{nullptr};
    Container* rightZone_{nullptr};
    Widget* centerWidget_{nullptr};
    std::vector<Widget*> actionWidgets_;

    CaptionButton hoveredBtn_{CaptionButton::None};
    CaptionButton pressedBtn_{CaptionButton::None};
    core::animation::PropertyAnimation<float> minHoverAnim_{0.0f};
    core::animation::PropertyAnimation<float> maxHoverAnim_{0.0f};
    core::animation::PropertyAnimation<float> closeHoverAnim_{0.0f};

    std::optional<core::Color> overrideBgColor_{std::nullopt};
    std::optional<core::Color> overrideBorderColor_{std::nullopt};
    std::optional<core::Color> overrideTitleColor_{std::nullopt};

    mutable std::optional<text::TextLayout> cachedTitleLayout_{std::nullopt};
    mutable float cachedTitleMaxWidth_{-1.0f};
    mutable bool titleCacheValid_{false};

    std::chrono::steady_clock::time_point lastCaptionClickTime_{};
    core::Point lastCaptionClickPos_{};
    bool hasLastCaptionClick_{false};

    bool isMaximized_{false};
};

} // namespace northern::widgets
