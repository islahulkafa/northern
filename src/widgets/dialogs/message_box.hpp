#pragma once

#include "widgets/dialogs/dialog.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace northern::widgets {

class Icon;
class Label;

enum class MessageBoxIcon : uint8_t { None, Info, Warning, Error, Confirm };

class MessageBox : public Dialog {
public:
    MessageBox(std::string title, std::string message,
               MessageBoxIcon iconType = MessageBoxIcon::Info,
               DialogButton buttons = DialogButton::Ok, std::shared_ptr<text::Font> font = nullptr);
    ~MessageBox() override = default;

    [[nodiscard]] MessageBoxIcon iconType() const noexcept { return iconType_; }
    void setIconType(MessageBoxIcon iconType);
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    void setMessage(std::string message);

    static MessageBox* info(WidgetContext& ctx, std::string title, std::string message,
                            std::function<void(DialogResult)> onFinishedCb = nullptr,
                            float dpiScale = 1.0f);

    static MessageBox* warning(WidgetContext& ctx, std::string title, std::string message,
                               std::function<void(DialogResult)> onFinishedCb = nullptr,
                               float dpiScale = 1.0f);

    static MessageBox* error(WidgetContext& ctx, std::string title, std::string message,
                             std::function<void(DialogResult)> onFinishedCb = nullptr,
                             float dpiScale = 1.0f);

    static MessageBox* confirm(WidgetContext& ctx, std::string title, std::string message,
                               std::function<void(DialogResult)> onFinishedCb = nullptr,
                               float dpiScale = 1.0f);

protected:
    void onDpiScaleChanged(float scale) override;
    void onFontChanged(const std::shared_ptr<text::Font>& font) override;

private:
    static MessageBox* showStandard(WidgetContext& ctx, std::string title, std::string message,
                                    MessageBoxIcon iconType, DialogButton buttons,
                                    std::function<void(DialogResult)> onFinishedCb, float dpiScale);

    MessageBoxIcon iconType_{MessageBoxIcon::Info};
    std::string message_{};
    Icon* iconWidget_{nullptr};
    Label* messageLabel_{nullptr};
};

} // namespace northern::widgets
