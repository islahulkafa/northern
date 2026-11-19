#pragma once

#include "renderer/svg/svg_document.hpp"
#include <memory>
#include <string_view>

namespace northern::widgets {

enum class IconId {
    // Window Controls
    WindowMinimize,
    WindowMaximize,
    WindowRestore,
    WindowClose,
    AppLogo,

    // Navigation & Chevrons
    ChevronDown,
    ChevronRight,
    ChevronUp,
    ChevronLeft,
    SortAscending,
    SortDescending,

    // Dialog & Status Badges
    DialogInfo,
    DialogWarning,
    DialogConfirm,
    DialogError,

    // Standard Desktop Actions
    Search,
    Folder,
    FolderOpen,
    File,
    Check,
    Close,
    Plus,
    Minus,
    Settings,
    Refresh,
    Trash
};

std::string_view getSvgString(IconId id) noexcept;
std::shared_ptr<renderer::SvgDocument> getSvgDocument(IconId id);

} // namespace northern::widgets
