#pragma once

#include "renderer/svg/svg_document.hpp"
#include <cstddef>
#include <memory>
#include <string_view>

namespace northern::widgets {

enum class IconId {
    // Window Controls
    WindowMinimize,
    WindowMaximize,
    WindowRestore,
    WindowClose,

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
    Trash,

    // Editing & Clipboard
    Edit,
    Copy,
    Cut,
    Paste,

    // File & Data Operations
    Save,
    Download,
    Upload,
    ExternalLink,

    // Media Controls
    Play,
    Pause,
    Stop,

    // State & Security
    Eye,
    EyeOff,
    Lock,
    Unlock,
    Bell,
    HelpCircle,

    // Navigation & Layout Views
    ArrowLeft,
    ArrowRight,
    ArrowUp,
    ArrowDown,
    Grid,
    List,
    Filter,
    MoreHorizontal,
    MoreVertical
};

inline constexpr size_t kTotalIconCount = static_cast<size_t>(IconId::MoreVertical) + 1;

std::string_view getSvgString(IconId id) noexcept;
std::shared_ptr<renderer::SvgDocument> getSvgDocument(IconId id);

} // namespace northern::widgets
