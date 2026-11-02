#include "WindowsFramelessHelper.h"

#include <QByteArray>
#include <QWindow>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>

void WindowsFramelessHelper::setWindow(QWindow *window)
{
    m_window = window;
    m_windowId = window ? window->winId() : 0;
    if (m_windowId) {
        // QML đã tạo cửa sổ: yêu cầu Windows tính lại non-client area ngay,
        // không chờ tới lần resize đầu tiên.
        SetWindowPos(reinterpret_cast<HWND>(m_windowId), nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

bool WindowsFramelessHelper::hasNoTitleBar() const
{
    if (!m_windowId || !m_window)
        return false;
    const HWND hwnd = reinterpret_cast<HWND>(m_windowId);
    RECT rect{};
    RECT client{};
    POINT origin{};
    return GetWindowRect(hwnd, &rect) && GetClientRect(hwnd, &client)
           && ClientToScreen(hwnd, &origin) && client.right > client.left
           && client.bottom > client.top && origin.y == rect.top;
}

bool WindowsFramelessHelper::nativeEventFilter(const QByteArray &eventType,
                                               void *message,
                                               qintptr *result)
{
    if (eventType != QByteArrayLiteral("windows_generic_MSG")
        && eventType != QByteArrayLiteral("windows_dispatcher_MSG"))
        return false;

    const MSG *msg = static_cast<MSG *>(message);
    if (!m_window || !m_windowId || msg->hwnd != reinterpret_cast<HWND>(m_windowId))
        return false;

    switch (msg->message) {
    case WM_NCCALCSIZE:
        if (msg->wParam == TRUE) {
            *result = 0;
            return true;
        }
        break;

    case WM_NCHITTEST: {
        if (IsZoomed(msg->hwnd))
            break;

        RECT rect;
        if (!GetWindowRect(msg->hwnd, &rect))
            break;

        using GetDpiForWindowFn = UINT(WINAPI *)(HWND);
        using GetSystemMetricsForDpiFn = int(WINAPI *)(int, UINT);
        const HMODULE user32 = GetModuleHandleW(L"user32.dll");
        static const auto getDpi = reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(user32, "GetDpiForWindow"));
        static const auto getMetrics = reinterpret_cast<GetSystemMetricsForDpiFn>(GetProcAddress(user32, "GetSystemMetricsForDpi"));
        const UINT dpi = getDpi ? getDpi(msg->hwnd) : 96;
        const int frameX = getMetrics ? getMetrics(SM_CXFRAME, dpi) + getMetrics(SM_CXPADDEDBORDER, dpi)
                                      : GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
        const int frameY = getMetrics ? getMetrics(SM_CYFRAME, dpi) + getMetrics(SM_CXPADDEDBORDER, dpi)
                                      : GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
        const int x = GET_X_LPARAM(msg->lParam);
        const int y = GET_Y_LPARAM(msg->lParam);
        const bool left = x >= rect.left && x < rect.left + frameX;
        const bool right = x < rect.right && x >= rect.right - frameX;
        const bool top = y >= rect.top && y < rect.top + frameY;
        const bool bottom = y < rect.bottom && y >= rect.bottom - frameY;

        if (top && left) *result = HTTOPLEFT;
        else if (top && right) *result = HTTOPRIGHT;
        else if (bottom && left) *result = HTBOTTOMLEFT;
        else if (bottom && right) *result = HTBOTTOMRIGHT;
        else if (left) *result = HTLEFT;
        else if (right) *result = HTRIGHT;
        else if (top) *result = HTTOP;
        else if (bottom) *result = HTBOTTOM;
        else break;
        return true;
    }

    case WM_GETMINMAXINFO: {
        // Không để cửa sổ maximize che taskbar hoặc tràn sang màn khác.
        const HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO info{};
        info.cbSize = sizeof(info);
        if (!monitor || !GetMonitorInfoW(monitor, &info))
            break;
        auto *limits = reinterpret_cast<MINMAXINFO *>(msg->lParam);
        limits->ptMaxPosition.x = info.rcWork.left - info.rcMonitor.left;
        limits->ptMaxPosition.y = info.rcWork.top - info.rcMonitor.top;
        limits->ptMaxSize.x = info.rcWork.right - info.rcWork.left;
        limits->ptMaxSize.y = info.rcWork.bottom - info.rcWork.top;
        limits->ptMaxTrackSize = limits->ptMaxSize;
        const QSize minimum = m_window->minimumSize();
        const qreal scale = m_window->devicePixelRatio();
        limits->ptMinTrackSize.x = qRound(minimum.width() * scale);
        limits->ptMinTrackSize.y = qRound(minimum.height() * scale);
        *result = 0;
        return true;
    }
    }
    return false;
}
