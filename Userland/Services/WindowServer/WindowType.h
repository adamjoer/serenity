/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

namespace WindowServer {

enum class WindowType {
    Invalid = 0,
    Normal,
    Menu,
    WindowSwitcher,
    Taskbar,
    Tooltip,
    Applet,
    Notification,
    Desktop,
    AppletArea,
    Popup,
    Autocomplete,
    _Count
};

}

template<>
struct AK::Formatter<WindowServer::WindowType> : AK::Formatter<FormatString> {
    ErrorOr<void> format(FormatBuilder& builder, WindowServer::WindowType value)
    {
        StringView value_name;
        switch (value) {
        case (WindowServer::WindowType::Invalid):
            value_name = "Invalid"sv;
            break;
        case (WindowServer::WindowType::Normal):
            value_name = "Normal"sv;
            break;
        case (WindowServer::WindowType::Menu):
            value_name = "Menu"sv;
            break;
        case (WindowServer::WindowType::WindowSwitcher):
            value_name = "WindowSwitcher"sv;
            break;
        case (WindowServer::WindowType::Taskbar):
            value_name = "Taskbar"sv;
            break;
        case (WindowServer::WindowType::Tooltip):
            value_name = "Tooltip"sv;
            break;
        case (WindowServer::WindowType::Applet):
            value_name = "Applet"sv;
            break;
        case (WindowServer::WindowType::Notification):
            value_name = "Notification"sv;
            break;
        case (WindowServer::WindowType::Desktop):
            value_name = "Desktop"sv;
            break;
        case (WindowServer::WindowType::AppletArea):
            value_name = "AppletArea"sv;
            break;
        case (WindowServer::WindowType::Popup):
            value_name = "Popup"sv;
            break;
        case (WindowServer::WindowType::Autocomplete):
            value_name = "Autocomplete"sv;
            break;
        case (WindowServer::WindowType::_Count):
            value_name = "_Count"sv;
            break;
        default:
            value_name = "??"sv;
            break;
        }
        return AK::Formatter<FormatString>::format(builder, "{}"sv, value_name);
    }
};
