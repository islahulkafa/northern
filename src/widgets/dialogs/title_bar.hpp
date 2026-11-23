#pragma once

#include "platform/window.hpp"
#include "text/font.hpp"
#include "widgets/controls/icon.hpp"
#include "widgets/base/widget.hpp"

#include <memory>
#include <string>
#include <vector>

namespace northern::widgets {

class Container;
class Label;

class TitleBar : public Widget {
public:
    static constexpr float kDefaultHeight = 36.0f;
    static constexpr float kCaptionButtonWidth = 44.0f;

    explicit TitleBar(std::string title = "Northern", std::shared_ptr<text::Font> font = nullptr,
                      platform::Window* window = nullptr);
    ~TitleBar() override = default;

    TitleBar(TitleBar&&) noexcept = default;
    TitleBar& operator=(TitleBar&&) noexcept = default;

    void setWindow(platform::Window* window);
    [[nodiscard]] platform::Window* window() const noexcept { return window_; }

    [[nodiscard]] const std::string& title() const noexcept { return title_; }
    void setTitle(std::string title);

    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    void setFont(std::shared_ptr<text::Font> font);

    void setCenterWidget(std::unique_ptr<Widget> widget);
    [[nodiscard]] Widget* centerWidget() const noexcept { return centerWidget_; }

    void addActionWidget(std::unique_ptr<Widget> widget);

    [[nodiscard]] platform::HitTestResult hitTestPoint(core::Point screenPos) const;

    void updateWindowState();

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

    std::string title_;
    std::shared_ptr<text::Font> font_;
    platform::Window* window_{nullptr};

    std::unique_ptr<Icon> appLogo_;
    std::unique_ptr<Icon> minIcon_;
    std::unique_ptr<Icon> maxIcon_;
    std::unique_ptr<Icon> restoreIcon_;
    std::unique_ptr<Icon> closeIcon_;

    Widget* centerWidget_{nullptr};
    std::vector<std::unique_ptr<Widget>> actionWidgets_;

    CaptionButton hoveredBtn_{CaptionButton::None};
    CaptionButton pressedBtn_{CaptionButton::None};

    bool isMaximized_{false};
};

} // namespace northern::widgets
