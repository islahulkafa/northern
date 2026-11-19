#include "platform/file_dialog.hpp"
#include "platform/window.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <shobjidl.h>
#include <windows.h>

namespace northern::platform {

namespace {

std::wstring utf8ToWide(std::string_view utf8Str) {
    if (utf8Str.empty()) {
        return L"";
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), static_cast<int>(utf8Str.size()),
                                  nullptr, 0);
    if (len <= 0) {
        return L"";
    }
    std::wstring result(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), static_cast<int>(utf8Str.size()), result.data(),
                        len);
    return result;
}

std::string wideToUtf8(std::wstring_view wideStr) {
    if (wideStr.empty()) {
        return "";
    }
    int len = WideCharToMultiByte(CP_UTF8, 0, wideStr.data(), static_cast<int>(wideStr.size()),
                                  nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return "";
    }
    std::string result(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wideStr.data(), static_cast<int>(wideStr.size()), result.data(),
                        len, nullptr, nullptr);
    return result;
}

struct ScopedCoInitialize {
    HRESULT hr;
    ScopedCoInitialize() {
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    }
    ~ScopedCoInitialize() {
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
    }
};

HWND getParentHwnd(Window* parent) {
    if (parent && parent->nativeHandle().isValid()) {
        return reinterpret_cast<HWND>(parent->nativeHandle().handle);
    }
    return nullptr;
}

} // namespace

core::Result<std::string, core::Error> FileDialog::openFile(Window* parent,
                                                            const FileDialogDesc& desc) {
    ScopedCoInitialize com;
    IFileOpenDialog* pFileOpen = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                                  reinterpret_cast<void**>(&pFileOpen));
    if (FAILED(hr)) {
        return core::Error(core::ErrorCode::PlatformError,
                           "Failed to create FileOpenDialog COM instance");
    }

    if (!desc.title.empty()) {
        std::wstring wTitle = utf8ToWide(desc.title);
        pFileOpen->SetTitle(wTitle.c_str());
    }

    DWORD dwFlags = 0;
    pFileOpen->GetOptions(&dwFlags);
    pFileOpen->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);

    std::vector<std::wstring> wideNames;
    std::vector<std::wstring> widePatterns;
    std::vector<COMDLG_FILTERSPEC> specs;
    wideNames.reserve(desc.filters.size());
    widePatterns.reserve(desc.filters.size());
    specs.reserve(desc.filters.size());

    for (const auto& filter : desc.filters) {
        wideNames.push_back(utf8ToWide(filter.name));
        widePatterns.push_back(utf8ToWide(filter.pattern));
        specs.push_back({wideNames.back().c_str(), widePatterns.back().c_str()});
    }

    if (!specs.empty()) {
        pFileOpen->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
    }

    HWND hwndParent = getParentHwnd(parent);
    hr = pFileOpen->Show(hwndParent);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) || hr == static_cast<HRESULT>(0x800704C7)) {
        pFileOpen->Release();
        return std::string("");
    }
    if (FAILED(hr)) {
        pFileOpen->Release();
        return core::Error(core::ErrorCode::PlatformError, "FileOpenDialog Show() failed");
    }

    IShellItem* pItem = nullptr;
    hr = pFileOpen->GetResult(&pItem);
    pFileOpen->Release();

    if (FAILED(hr) || !pItem) {
        return core::Error(core::ErrorCode::PlatformError, "Failed to retrieve selected item");
    }

    PWSTR pszFilePath = nullptr;
    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    pItem->Release();

    if (FAILED(hr) || !pszFilePath) {
        return core::Error(core::ErrorCode::PlatformError, "Failed to get item file system path");
    }

    std::string result = wideToUtf8(pszFilePath);
    CoTaskMemFree(pszFilePath);
    return result;
}

core::Result<std::vector<std::string>, core::Error>
FileDialog::openFiles(Window* parent, const FileDialogDesc& desc) {
    ScopedCoInitialize com;
    IFileOpenDialog* pFileOpen = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                                  reinterpret_cast<void**>(&pFileOpen));
    if (FAILED(hr)) {
        return core::Error(core::ErrorCode::PlatformError,
                           "Failed to create FileOpenDialog instance");
    }

    if (!desc.title.empty()) {
        std::wstring wTitle = utf8ToWide(desc.title);
        pFileOpen->SetTitle(wTitle.c_str());
    }

    DWORD dwFlags = 0;
    pFileOpen->GetOptions(&dwFlags);
    pFileOpen->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_ALLOWMULTISELECT);

    std::vector<std::wstring> wideNames;
    std::vector<std::wstring> widePatterns;
    std::vector<COMDLG_FILTERSPEC> specs;
    for (const auto& filter : desc.filters) {
        wideNames.push_back(utf8ToWide(filter.name));
        widePatterns.push_back(utf8ToWide(filter.pattern));
        specs.push_back({wideNames.back().c_str(), widePatterns.back().c_str()});
    }
    if (!specs.empty()) {
        pFileOpen->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
    }

    HWND hwndParent = getParentHwnd(parent);
    hr = pFileOpen->Show(hwndParent);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) || hr == static_cast<HRESULT>(0x800704C7)) {
        pFileOpen->Release();
        return std::vector<std::string>{};
    }
    if (FAILED(hr)) {
        pFileOpen->Release();
        return core::Error(core::ErrorCode::PlatformError, "FileOpenDialog Show() failed");
    }

    IShellItemArray* pItems = nullptr;
    hr = pFileOpen->GetResults(&pItems);
    pFileOpen->Release();
    if (FAILED(hr) || !pItems) {
        return core::Error(core::ErrorCode::PlatformError, "Failed to get selected items array");
    }

    DWORD count = 0;
    pItems->GetCount(&count);
    std::vector<std::string> results;
    results.reserve(count);

    for (DWORD i = 0; i < count; ++i) {
        IShellItem* pItem = nullptr;
        if (SUCCEEDED(pItems->GetItemAt(i, &pItem)) && pItem) {
            PWSTR pszPath = nullptr;
            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)) && pszPath) {
                results.push_back(wideToUtf8(pszPath));
                CoTaskMemFree(pszPath);
            }
            pItem->Release();
        }
    }
    pItems->Release();
    return results;
}

core::Result<std::string, core::Error> FileDialog::saveFile(Window* parent,
                                                            const FileDialogDesc& desc) {
    ScopedCoInitialize com;
    IFileSaveDialog* pFileSave = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_IFileSaveDialog,
                                  reinterpret_cast<void**>(&pFileSave));
    if (FAILED(hr)) {
        return core::Error(core::ErrorCode::PlatformError,
                           "Failed to create FileSaveDialog instance");
    }

    if (!desc.title.empty()) {
        std::wstring wTitle = utf8ToWide(desc.title);
        pFileSave->SetTitle(wTitle.c_str());
    }

    if (!desc.defaultFilename.empty()) {
        std::wstring wDefName = utf8ToWide(desc.defaultFilename);
        pFileSave->SetFileName(wDefName.c_str());
    }

    DWORD dwFlags = 0;
    pFileSave->GetOptions(&dwFlags);
    pFileSave->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);

    std::vector<std::wstring> wideNames;
    std::vector<std::wstring> widePatterns;
    std::vector<COMDLG_FILTERSPEC> specs;
    for (const auto& filter : desc.filters) {
        wideNames.push_back(utf8ToWide(filter.name));
        widePatterns.push_back(utf8ToWide(filter.pattern));
        specs.push_back({wideNames.back().c_str(), widePatterns.back().c_str()});
    }
    if (!specs.empty()) {
        pFileSave->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
    }

    HWND hwndParent = getParentHwnd(parent);
    hr = pFileSave->Show(hwndParent);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) || hr == static_cast<HRESULT>(0x800704C7)) {
        pFileSave->Release();
        return std::string("");
    }
    if (FAILED(hr)) {
        pFileSave->Release();
        return core::Error(core::ErrorCode::PlatformError, "FileSaveDialog Show() failed");
    }

    IShellItem* pItem = nullptr;
    hr = pFileSave->GetResult(&pItem);
    pFileSave->Release();
    if (FAILED(hr) || !pItem) {
        return core::Error(core::ErrorCode::PlatformError, "Failed to get saved item result");
    }

    PWSTR pszPath = nullptr;
    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
    pItem->Release();
    if (FAILED(hr) || !pszPath) {
        return core::Error(core::ErrorCode::PlatformError, "Failed to get saved item path");
    }

    std::string result = wideToUtf8(pszPath);
    CoTaskMemFree(pszPath);
    return result;
}

core::Result<std::string, core::Error> FileDialog::selectFolder(Window* parent,
                                                                const FileDialogDesc& desc) {
    ScopedCoInitialize com;
    IFileOpenDialog* pFileOpen = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                                  reinterpret_cast<void**>(&pFileOpen));
    if (FAILED(hr)) {
        return core::Error(core::ErrorCode::PlatformError,
                           "Failed to create FileOpenDialog instance for folders");
    }

    if (!desc.title.empty()) {
        std::wstring wTitle = utf8ToWide(desc.title);
        pFileOpen->SetTitle(wTitle.c_str());
    }

    DWORD dwFlags = 0;
    pFileOpen->GetOptions(&dwFlags);
    pFileOpen->SetOptions(dwFlags | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);

    HWND hwndParent = getParentHwnd(parent);
    hr = pFileOpen->Show(hwndParent);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) || hr == static_cast<HRESULT>(0x800704C7)) {
        pFileOpen->Release();
        return std::string("");
    }
    if (FAILED(hr)) {
        pFileOpen->Release();
        return core::Error(core::ErrorCode::PlatformError, "Folder picker Show() failed");
    }

    IShellItem* pItem = nullptr;
    hr = pFileOpen->GetResult(&pItem);
    pFileOpen->Release();
    if (FAILED(hr) || !pItem) {
        return core::Error(core::ErrorCode::PlatformError, "Failed to get selected folder item");
    }

    PWSTR pszPath = nullptr;
    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
    pItem->Release();
    if (FAILED(hr) || !pszPath) {
        return core::Error(core::ErrorCode::PlatformError, "Failed to get selected folder path");
    }

    std::string result = wideToUtf8(pszPath);
    CoTaskMemFree(pszPath);
    return result;
}

} // namespace northern::platform
