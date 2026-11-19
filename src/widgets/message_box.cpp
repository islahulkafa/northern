#include "widgets/message_box.hpp"
#include "widgets/dialog.hpp"
#include "widgets/theme.hpp"

namespace northern::widgets {

void MessageBox::showInformation(WidgetContext& ctx, std::string title, std::string message,
                                 std::shared_ptr<text::Font> font,
                                 std::function<void()> onDismiss) {
    auto dialog = std::make_unique<Dialog>(std::move(title), font);
    dialog->setStatusIcon(IconId::DialogInfo, theme::ColorTokens::AccentDefault);
    dialog->setMessage(std::move(message));

    Dialog* rawDialog = dialog.get();
    if (onDismiss) {
        rawDialog->onAccepted.connect(onDismiss);
        rawDialog->onRejected.connect(onDismiss);
    }

    rawDialog->addActionButton("OK", ButtonVariant::Primary,
                               [rawDialog]() { rawDialog->accept(); });

    dialog.release()->open(ctx);
}

void MessageBox::showConfirm(WidgetContext& ctx, std::string title, std::string message,
                             std::shared_ptr<text::Font> font,
                             std::function<void(bool confirmed)> onResponse) {
    auto dialog = std::make_unique<Dialog>(std::move(title), font);
    dialog->setStatusIcon(IconId::DialogConfirm, core::Color::fromRgba8(16, 185, 129));
    dialog->setMessage(std::move(message));

    Dialog* rawDialog = dialog.get();
    if (onResponse) {
        rawDialog->onAccepted.connect([onResponse]() { onResponse(true); });
        rawDialog->onRejected.connect([onResponse]() { onResponse(false); });
    }

    rawDialog->addActionButton("Cancel", ButtonVariant::Secondary,
                               [rawDialog]() { rawDialog->reject(); });

    rawDialog->addActionButton("Confirm", ButtonVariant::Primary,
                               [rawDialog]() { rawDialog->accept(); });

    dialog.release()->open(ctx);
}

void MessageBox::showWarning(WidgetContext& ctx, std::string title, std::string message,
                             std::shared_ptr<text::Font> font,
                             std::function<void(bool proceed)> onResponse) {
    auto dialog = std::make_unique<Dialog>(std::move(title), font);
    dialog->setStatusIcon(IconId::DialogWarning, core::Color::fromRgba8(245, 158, 11));
    dialog->setMessage(std::move(message));

    Dialog* rawDialog = dialog.get();
    if (onResponse) {
        rawDialog->onAccepted.connect([onResponse]() { onResponse(true); });
        rawDialog->onRejected.connect([onResponse]() { onResponse(false); });
    }

    rawDialog->addActionButton("Cancel", ButtonVariant::Secondary,
                               [rawDialog]() { rawDialog->reject(); });

    rawDialog->addActionButton("Proceed", ButtonVariant::Destructive,
                               [rawDialog]() { rawDialog->accept(); });

    dialog.release()->open(ctx);
}

} // namespace northern::widgets
