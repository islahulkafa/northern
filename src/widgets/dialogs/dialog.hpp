#pragma once

#include "core/signal/signal.hpp"
#include "text/font.hpp"
#include "widgets/base/widget.hpp"
#include "widgets/containers/container.hpp"
#include "widgets/controls/button.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace northern::widgets {

class Label;
class WidgetContext;

enum class DialogResult : uint8_t { Accepted, Rejected, Custom };

enum class DialogButton : uint32_t {
    None = 0,
    Ok = 1u << 0,
    Cancel = 1u << 1,
    Yes = 1u << 2,
    No = 1u << 3,
    Apply = 1u << 4,
    Close = 1u << 5,
};

[[nodiscard]] constexpr DialogButton operator|(DialogButton a, DialogButton b) noexcept {
    return static_cast<DialogButton>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

[[nodiscard]] constexpr DialogButton operator&(DialogButton a, DialogButton b) noexcept {
    return static_cast<DialogButton>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

[[nodiscard]] constexpr bool hasDialogButton(DialogButton mask, DialogButton flag) noexcept {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(flag)) != 0;
}

class Dialog : public Container {
public:
    explicit Dialog(std::string title = "", std::shared_ptr<text::Font> font = nullptr);
    ~Dialog() override;

    static Dialog* show(WidgetContext& ctx, std::unique_ptr<Dialog> dialog);
    void open(WidgetContext& ctx);

    void accept();
    void reject();
    void close(DialogResult result = DialogResult::Rejected);

    [[nodiscard]] DialogResult result() const noexcept { return result_; }
    [[nodiscard]] bool isOpen() const noexcept { return isOpen_; }

    void setTitle(std::string title);
    [[nodiscard]] const std::string& title() const noexcept { return title_; }

    void setFont(std::shared_ptr<text::Font> font);
    [[nodiscard]] std::shared_ptr<text::Font> font() const noexcept { return font_; }
    [[nodiscard]] std::shared_ptr<text::Font> effectiveFont() const;

    void setDpiScale(float scale);
    [[nodiscard]] float dpiScale() const noexcept { return dpiScale_; }

    void setDialogWidth(float width);
    [[nodiscard]] float dialogWidth() const noexcept { return dialogWidth_; }

    void setDismissOnEscape(bool dismiss) noexcept { dismissOnEscape_ = dismiss; }
    [[nodiscard]] bool dismissOnEscape() const noexcept { return dismissOnEscape_; }

    void setDismissOnOutsideClick(bool dismiss) noexcept { dismissOnOutsideClick_ = dismiss; }
    [[nodiscard]] bool dismissOnOutsideClick() const noexcept { return dismissOnOutsideClick_; }

    void setCloseButtonVisible(bool visible);
    [[nodiscard]] bool isCloseButtonVisible() const noexcept { return closeButtonVisible_; }

    void setContent(std::unique_ptr<Widget> content);
    [[nodiscard]] Container* contentContainer() noexcept { return contentContainer_; }
    [[nodiscard]] const Container* contentContainer() const noexcept { return contentContainer_; }

    void setStandardButtons(DialogButton buttons);
    [[nodiscard]] DialogButton standardButtons() const noexcept { return standardButtons_; }
    [[nodiscard]] Button* button(DialogButton which) const noexcept;

    void setDefaultButton(Button* btn) noexcept { defaultButton_ = btn; }
    [[nodiscard]] Button* defaultButton() const noexcept { return defaultButton_; }

    Button* addButton(std::string text, ButtonVariant variant = ButtonVariant::Secondary,
                      DialogResult result = DialogResult::Custom);

    void onKeyDown(WidgetKeyEvent& ev) override;
    void onContextAttached() override;
    void onThemeChanged() override;

    core::Signal<> onAccepted;
    core::Signal<> onRejected;
    core::Signal<DialogResult> onFinished;

protected:
    virtual void onDpiScaleChanged(float scale);
    virtual void onFontChanged(const std::shared_ptr<text::Font>& font);

private:
    void applyDpiScale(float scale);
    void applyMetrics();
    void configureOverlayAndShow(WidgetContext& ctx, bool takeOwnership,
                                 std::unique_ptr<Dialog> ownedPtr);
    void handleExternalDismiss();

    std::string title_{};
    std::shared_ptr<text::Font> font_{nullptr};
    mutable std::shared_ptr<text::Font> resolvedDefaultFont_{nullptr};
    float dpiScale_{1.0f};
    float dialogWidth_{420.0f};

    Container* headerRow_{nullptr};
    Label* titleLabel_{nullptr};
    Button* closeButton_{nullptr};
    Container* contentContainer_{nullptr};
    Container* footerRow_{nullptr};

    DialogButton standardButtons_{DialogButton::None};
    std::vector<std::pair<DialogButton, Button*>> buttonEntries_{};
    Button* defaultButton_{nullptr};

    DialogResult result_{DialogResult::Rejected};
    bool dismissOnEscape_{true};
    bool dismissOnOutsideClick_{false};
    bool closeButtonVisible_{true};
    bool hasCustomDpiScale_{false};
    bool isOpen_{false};
    bool isClosing_{false};
};

} // namespace northern::widgets
