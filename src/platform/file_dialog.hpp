#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include <string>
#include <vector>

namespace northern::platform {

class Window;

struct FileFilter {
    std::string name;    // e.g. "Image Files (*.png;*.jpg)"
    std::string pattern; // e.g. "*.png;*.jpg"
};

struct FileDialogDesc {
    std::string title;
    std::string defaultPath;
    std::string defaultFilename;
    std::vector<FileFilter> filters;
    bool allowMultiSelect{false};
};

class FileDialog {
public:
    static core::Result<std::string, core::Error> openFile(Window* parent = nullptr,
                                                           const FileDialogDesc& desc = {});

    static core::Result<std::vector<std::string>, core::Error>
    openFiles(Window* parent = nullptr, const FileDialogDesc& desc = {});

    static core::Result<std::string, core::Error> saveFile(Window* parent = nullptr,
                                                           const FileDialogDesc& desc = {});

    static core::Result<std::string, core::Error> selectFolder(Window* parent = nullptr,
                                                               const FileDialogDesc& desc = {});
};

} // namespace northern::platform
