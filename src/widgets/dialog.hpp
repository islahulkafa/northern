#pragma once

#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/button.hpp"
#include "widgets/container.hpp"
#include "widgets/icon.hpp"
#include "widgets/label.hpp"
#include "widgets/widget.hpp"
#include "widgets/widget_context.hpp"

#include <functional>
#include <memory>
#include <string>

namespace northern::widgets {

enum class DialogResult { Accepted, Rejected, Custom };

class Dialog : public Widget {
public:
    explicit Dialog(std::string title = "Dialog", std::shared_ptr<text::Font> font = nullptr);
    ~Dialog() override = default;

    Dialog(Dialog&&) noexcept = default;
    Dialog& operator=(Dialog&&) noexcept = default;

    [[nodiscard]] const std::string& title() const noexcept { return title_; }
    void setTitle(std::string title);

    void setStatusIcon(IconId iconId, core::Color color);
    void clearStatusIcon();

    void setMessage(std::string message);
    [[nodiscard]] const std::string& message() const noexcept { return message_; }

    template <typename T>
    T* setCustomContent(std::unique_ptr<T> content) {
        T* raw = content.get();
        setCustomContentInternal(std::move(content));
        return raw;
    }

    [[nodiscard]] Container* bodyContainer() const noexcept { return bodyContainer_; }
    [[nodiscard]] Container* footerContainer() const noexcept { return footerContainer_; }

    void addActionButton(std::string text, ButtonVariant variant = ButtonVariant::Secondary,
                         std::function<void()> onClicked = nullptr);

    void open(WidgetContext& ctx, bool lightDismiss = false);
    void accept();
    void reject();

    void paint(renderer::PaintContext& ctx) override;
    void onKeyDown(WidgetKeyEvent& ev) override;

    core::Signal<DialogResult> onFinished;
    core::Signal<> onAccepted;
    core::Signal<> onRejected;

private:
    void setCustomContentInternal(std::unique_ptr<Widget> content);

    std::string title_;
    std::string message_;
    std::shared_ptr<text::Font> font_;

    Container* headerContainer_{nullptr};
    Icon* statusIconWidget_{nullptr};
    Label* titleLabel_{nullptr};
    Container* bodyContainer_{nullptr};
    Label* messageLabel_{nullptr};
    Container* footerContainer_{nullptr};

    WidgetContext* activeContext_{nullptr};
};

} // namespace northern::widgets
