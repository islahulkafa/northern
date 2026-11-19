#include "widgets/icons.hpp"

#include <array>
#include <mutex>

namespace northern::widgets {

std::string_view getSvgString(IconId id) noexcept {
    switch (id) {
    case IconId::WindowMinimize:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"4\" y1=\"12\" "
               "x2=\"20\" y2=\"12\"/></svg>";
    case IconId::WindowMaximize:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"4\" y=\"4\" "
               "width=\"16\" height=\"16\" rx=\"2\"/></svg>";
    case IconId::WindowRestore:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M8 4h10a2 2 0 0 1 2 "
               "2v10\"/><rect x=\"4\" y=\"8\" width=\"12\" height=\"12\" rx=\"2\"/></svg>";
    case IconId::WindowClose:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"18\" y1=\"6\" "
               "x2=\"6\" y2=\"18\"/><line x1=\"6\" y1=\"6\" x2=\"18\" y2=\"18\"/></svg>";
    case IconId::AppLogo:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><polygon points=\"12 2 2 22 22 "
               "22\"/><circle cx=\"12\" cy=\"14\" r=\"3\"/></svg>";

    case IconId::ChevronDown:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"6 9 12 15 18 "
               "9\"/></svg>";
    case IconId::ChevronRight:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"9 18 15 12 9 "
               "6\"/></svg>";
    case IconId::ChevronUp:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"18 15 12 9 6 "
               "15\"/></svg>";
    case IconId::ChevronLeft:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"15 18 9 12 "
               "15 6\"/></svg>";
    case IconId::SortAscending:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"12\" y1=\"19\" "
               "x2=\"12\" y2=\"5\"/><polyline points=\"5 12 12 5 19 12\"/></svg>";
    case IconId::SortDescending:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"12\" y1=\"5\" "
               "x2=\"12\" y2=\"19\"/><polyline points=\"19 12 12 19 5 12\"/></svg>";

    case IconId::DialogInfo:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" "
               "r=\"10\"/><line x1=\"12\" y1=\"16\" x2=\"12\" y2=\"12\"/><line x1=\"12\" y1=\"8\" "
               "x2=\"12.01\" y2=\"8\"/></svg>";
    case IconId::DialogWarning:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M10.29 3.86L1.82 "
               "18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z\"/><line "
               "x1=\"12\" y1=\"9\" x2=\"12\" y2=\"13\"/><line x1=\"12\" y1=\"17\" x2=\"12.01\" "
               "y2=\"17\"/></svg>";
    case IconId::DialogConfirm:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" "
               "r=\"10\"/><polyline points=\"9 12 11 14 15 10\"/></svg>";
    case IconId::DialogError:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" "
               "r=\"10\"/><line x1=\"15\" y1=\"9\" x2=\"9\" y2=\"15\"/><line x1=\"9\" y1=\"9\" "
               "x2=\"15\" y2=\"15\"/></svg>";

    case IconId::Search:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"11\" cy=\"11\" "
               "r=\"8\"/><line x1=\"21\" y1=\"21\" x2=\"16.65\" y2=\"16.65\"/></svg>";
    case IconId::Folder:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M22 19a2 2 0 0 1-2 "
               "2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z\"/></svg>";
    case IconId::FolderOpen:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M22 19a2 2 0 0 1-2 "
               "2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z\"/><polygon points=\"2 "
               "10 22 10 20 21 4 21\"/></svg>";
    case IconId::File:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M13 2H6a2 2 0 0 0-2 "
               "2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z\"/><polyline points=\"13 2 13 9 20 "
               "9\"/></svg>";
    case IconId::Check:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"20 6 9 17 4 "
               "12\"/></svg>";
    case IconId::Close:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"18\" y1=\"6\" "
               "x2=\"6\" y2=\"18\"/><line x1=\"6\" y1=\"6\" x2=\"18\" y2=\"18\"/></svg>";
    case IconId::Plus:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"12\" y1=\"5\" "
               "x2=\"12\" y2=\"19\"/><line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"/></svg>";
    case IconId::Minus:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"5\" y1=\"12\" "
               "x2=\"19\" y2=\"12\"/></svg>";
    case IconId::Settings:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" "
               "r=\"3\"/><path d=\"M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 "
               "0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 "
               "0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 "
               "0-1.82.33l-.06.06a2 "
               "2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 "
               "0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 "
               "0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 "
               "0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 "
               "0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 "
               "2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 "
               "2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z\"/></svg>";
    case IconId::Refresh:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"23 4 23 10 "
               "17 10\"/><polyline points=\"1 20 1 14 7 14\"/><path d=\"M3.51 9a9 9 0 0 1 "
               "14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15\"/></svg>";
    case IconId::Trash:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"3 6 5 6 21 "
               "6\"/><path d=\"M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 "
               "2 0 0 1 2 2v2\"/></svg>";
    }
    return "";
}

std::shared_ptr<renderer::SvgDocument> getSvgDocument(IconId id) {
    constexpr size_t kIconCount = static_cast<size_t>(IconId::Trash) + 1;
    static std::array<std::shared_ptr<renderer::SvgDocument>, kIconCount> s_cache;
    static std::mutex s_mutex;

    size_t index = static_cast<size_t>(id);
    if (index >= kIconCount) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_cache[index]) {
        std::string_view svg = getSvgString(id);
        if (!svg.empty()) {
            auto res = renderer::SvgDocument::fromString(svg);
            if (res.isOk()) {
                s_cache[index] = res.value();
            }
        }
    }
    return s_cache[index];
}

} // namespace northern::widgets
