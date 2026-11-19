#pragma once

#include "text/font.hpp"
#include "widgets/widget_context.hpp"

#include <functional>
#include <memory>
#include <string>

namespace northern::widgets {

class MessageBox {
public:
    static void showInformation(WidgetContext& ctx, std::string title, std::string message,
                                std::shared_ptr<text::Font> font,
                                std::function<void()> onDismiss = nullptr);

    static void showConfirm(WidgetContext& ctx, std::string title, std::string message,
                            std::shared_ptr<text::Font> font,
                            std::function<void(bool confirmed)> onResponse = nullptr);

    static void showWarning(WidgetContext& ctx, std::string title, std::string message,
                            std::shared_ptr<text::Font> font,
                            std::function<void(bool proceed)> onResponse = nullptr);
};

} // namespace northern::widgets
