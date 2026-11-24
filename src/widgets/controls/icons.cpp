#include "widgets/controls/icons.hpp"

#include <array>
#include <atomic>
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

    // Editing & Clipboard
    case IconId::Edit:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M11 4H4a2 2 0 0 0-2 "
               "2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7\"/><path d=\"M18.5 2.5a2.121 2.121 0 0 1 "
               "3 3L12 15l-4 1 1-4 9.5-9.5z\"/></svg>";
    case IconId::Copy:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"9\" y=\"9\" "
               "width=\"13\" "
               "height=\"13\" rx=\"2\" ry=\"2\"/><path d=\"M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 "
               "2-2h9a2 2 0 0 1 2 2v1\"/></svg>";
    case IconId::Cut:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"6\" cy=\"6\" "
               "r=\"3\"/><circle cx=\"6\" cy=\"18\" r=\"3\"/><line x1=\"20\" y1=\"4\" x2=\"8.12\" "
               "y2=\"15.88\"/><line x1=\"14.47\" y1=\"14.48\" x2=\"20\" y2=\"20\"/><line "
               "x1=\"8.12\" "
               "y1=\"8.12\" x2=\"12\" y2=\"12\"/></svg>";
    case IconId::Paste:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M16 4h2a2 2 0 0 1 2 "
               "2v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2h2\"/><rect x=\"8\" y=\"2\" "
               "width=\"8\" height=\"4\" rx=\"1\" ry=\"1\"/></svg>";

    // File & Data Operations
    case IconId::Save:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M19 21H5a2 2 0 0 "
               "1-2-2V5a2 "
               "2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z\"/><polyline points=\"17 21 17 13 7 13 7 "
               "21\"/><polyline points=\"7 3 7 8 15 8\"/></svg>";
    case IconId::Download:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M21 15v4a2 2 0 0 1-2 "
               "2H5a2 2 0 0 1-2-2v-4\"/><polyline points=\"7 10 12 15 17 10\"/><line x1=\"12\" "
               "y1=\"15\" x2=\"12\" y2=\"3\"/></svg>";
    case IconId::Upload:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M21 15v4a2 2 0 0 1-2 "
               "2H5a2 2 0 0 1-2-2v-4\"/><polyline points=\"17 8 12 3 7 8\"/><line x1=\"12\" "
               "y1=\"3\" "
               "x2=\"12\" y2=\"15\"/></svg>";
    case IconId::ExternalLink:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M18 13v6a2 2 0 0 1-2 "
               "2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6\"/><polyline points=\"15 3 21 3 21 9\"/><line "
               "x1=\"10\" y1=\"14\" x2=\"21\" y2=\"3\"/></svg>";

    // Media Controls
    case IconId::Play:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><polygon points=\"5 3 19 12 5 "
               "21 5 "
               "3\"/></svg>";
    case IconId::Pause:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"6\" y=\"4\" "
               "width=\"4\" "
               "height=\"16\"/><rect x=\"14\" y=\"4\" width=\"4\" height=\"16\"/></svg>";
    case IconId::Stop:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"4\" y=\"4\" "
               "width=\"16\" "
               "height=\"16\" rx=\"2\"/></svg>";

    // State & Security
    case IconId::Eye:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M1 12s4-8 11-8 11 8 "
               "11 "
               "8-4 8-11 8-11-8-11-8z\"/><circle cx=\"12\" cy=\"12\" r=\"3\"/></svg>";
    case IconId::EyeOff:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M17.94 17.94A10.07 "
               "10.07 "
               "0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 "
               "4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24\"/><line "
               "x1=\"1\" y1=\"1\" x2=\"23\" y2=\"23\"/></svg>";
    case IconId::Lock:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"3\" y=\"11\" "
               "width=\"18\" "
               "height=\"11\" rx=\"2\" ry=\"2\"/><path d=\"M7 11V7a5 5 0 0 1 10 0v4\"/></svg>";
    case IconId::Unlock:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"3\" y=\"11\" "
               "width=\"18\" "
               "height=\"11\" rx=\"2\" ry=\"2\"/><path d=\"M7 11V7a5 5 0 0 1 9.9-1\"/></svg>";
    case IconId::Bell:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M18 8A6 6 0 0 0 6 8c0 "
               "7-3 "
               "9-3 9h18s-3-2-3-9\"/><path d=\"M13.73 21a2 2 0 0 1-3.46 0\"/></svg>";
    case IconId::HelpCircle:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" "
               "r=\"10\"/><path d=\"M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3\"/><line x1=\"12\" "
               "y1=\"17\" "
               "x2=\"12.01\" y2=\"17\"/></svg>";

    // Navigation & Layout Views
    case IconId::ArrowLeft:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"19\" y1=\"12\" "
               "x2=\"5\" "
               "y2=\"12\"/><polyline points=\"12 19 5 12 12 5\"/></svg>";
    case IconId::ArrowRight:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"5\" y1=\"12\" "
               "x2=\"19\" "
               "y2=\"12\"/><polyline points=\"12 5 19 12 12 19\"/></svg>";
    case IconId::ArrowUp:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"12\" y1=\"19\" "
               "x2=\"12\" "
               "y2=\"5\"/><polyline points=\"5 12 12 5 19 12\"/></svg>";
    case IconId::ArrowDown:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"12\" y1=\"5\" "
               "x2=\"12\" "
               "y2=\"19\"/><polyline points=\"19 12 12 19 5 12\"/></svg>";
    case IconId::Grid:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><rect x=\"3\" y=\"3\" "
               "width=\"7\" "
               "height=\"7\"/><rect x=\"14\" y=\"3\" width=\"7\" height=\"7\"/><rect x=\"14\" "
               "y=\"14\" width=\"7\" height=\"7\"/><rect x=\"3\" y=\"14\" width=\"7\" "
               "height=\"7\"/></svg>";
    case IconId::List:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><line x1=\"8\" y1=\"6\" "
               "x2=\"21\" "
               "y2=\"6\"/><line x1=\"8\" y1=\"12\" x2=\"21\" y2=\"12\"/><line x1=\"8\" y1=\"18\" "
               "x2=\"21\" y2=\"18\"/><line x1=\"3\" y1=\"6\" x2=\"3.01\" y2=\"6\"/><line x1=\"3\" "
               "y1=\"12\" x2=\"3.01\" y2=\"12\"/><line x1=\"3\" y1=\"18\" x2=\"3.01\" "
               "y2=\"18\"/></svg>";
    case IconId::Filter:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><polygon points=\"22 3 2 3 10 "
               "12.46 10 19 14 21 14 12.46 22 3\"/></svg>";
    case IconId::MoreHorizontal:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" "
               "r=\"1\"/><circle cx=\"19\" cy=\"12\" r=\"1\"/><circle cx=\"5\" cy=\"12\" "
               "r=\"1\"/></svg>";
    case IconId::MoreVertical:
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 "
               "0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" "
               "r=\"1\"/><circle cx=\"12\" cy=\"5\" r=\"1\"/><circle cx=\"12\" cy=\"19\" "
               "r=\"1\"/></svg>";
    }
    return "";
}

std::shared_ptr<renderer::SvgDocument> getSvgDocument(IconId id) {
    size_t index = static_cast<size_t>(id);
    if (index >= kTotalIconCount) {
        return nullptr;
    }

    static std::array<std::shared_ptr<renderer::SvgDocument>, kTotalIconCount> s_cache;
    static std::array<std::atomic<renderer::SvgDocument*>, kTotalIconCount> s_atomicPtrs{};
    static std::mutex s_mutex;

    // Fast-path: acquire load on atomic pointer
    renderer::SvgDocument* rawDoc = s_atomicPtrs[index].load(std::memory_order_acquire);
    if (rawDoc != nullptr) {
        return s_cache[index];
    }

    // Slow-path: parse under mutex
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_cache[index]) {
        std::string_view svg = getSvgString(id);
        if (!svg.empty()) {
            auto res = renderer::SvgDocument::fromString(svg);
            if (res.isOk()) {
                s_cache[index] = res.value();
                s_atomicPtrs[index].store(s_cache[index].get(), std::memory_order_release);
            }
        }
    }
    return s_cache[index];
}

} // namespace northern::widgets
