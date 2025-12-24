#include "TooltipWindow.h"
#include <windowsx.h>

bool TooltipWindow::s_classRegistered = false;

TooltipWindow::TooltipWindow() = default;

TooltipWindow::~TooltipWindow() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
}

bool TooltipWindow::create(HWND parent) {
    m_parent = parent;

    // Register window class once
    if (!s_classRegistered) {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wc.lpszClassName = L"CustomTooltipWindow";

        if (!RegisterClassEx(&wc)) {
            return false;
        }
        s_classRegistered = true;
    }

    // Create popup window with topmost flag
    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        L"CustomTooltipWindow",
        L"",
        WS_POPUP,
        0, 0, 100, 30,
        parent,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );

    return m_hwnd != nullptr;
}

void TooltipWindow::show(const std::wstring& text, int screenX, int screenY, bool positionAbove) {
    if (!m_hwnd) return;

    m_text = text;
    updateSize(text);

    // Position tooltip centered horizontally on the point
    int x = screenX - m_width / 2;

    // Position vertically: above mouse cursor by default with 20px padding
    int y;
    if (positionAbove) {
        y = screenY - m_height - 20;
    } else {
        y = screenY + 20;
    }

    // Keep tooltip on screen
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    if (x < workArea.left) x = workArea.left;
    if (x + m_width > workArea.right) x = workArea.right - m_width;

    // If positioning above would go off top of screen, position below instead
    if (y < workArea.top) {
        y = screenY + 20;
    }
    // If positioning below would go off bottom of screen, position above instead
    if (y + m_height > workArea.bottom) {
        y = screenY - m_height - 20;
    }

    SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, m_width, m_height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void TooltipWindow::hide() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

void TooltipWindow::updateSize(const std::wstring& text) {
    HDC hdc = GetDC(m_hwnd);

    // Use standard tooltip font
    NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    HFONT hFont = CreateFontIndirect(&ncm.lfStatusFont);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    // Measure text
    RECT textRect = { 0, 0, 0, 0 };
    DrawText(hdc, text.c_str(), -1, &textRect, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    ReleaseDC(m_hwnd, hdc);

    // Add padding
    int padding = 6;
    m_width = textRect.right - textRect.left + padding * 2;
    m_height = textRect.bottom - textRect.top + padding * 2;
}

void TooltipWindow::render() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    // Get client rect
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    // Fill background with tooltip color (light yellow #FFFFE1)
    HBRUSH bgBrush = CreateSolidBrush(RGB(255, 255, 225));
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);

    // Draw border (black)
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    Rectangle(hdc, 0, 0, rc.right, rc.bottom);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);

    // Draw text (black)
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));

    // Use standard tooltip font
    NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    HFONT hFont = CreateFontIndirect(&ncm.lfStatusFont);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    // Center text with padding
    RECT textRect = rc;
    textRect.left += 6;
    textRect.top += 6;
    textRect.right -= 6;
    textRect.bottom -= 6;

    DrawText(hdc, m_text.c_str(), -1, &textRect, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    EndPaint(m_hwnd, &ps);
}

LRESULT CALLBACK TooltipWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    TooltipWindow* window = nullptr;

    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = static_cast<TooltipWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->m_hwnd = hwnd;
    } else {
        window = reinterpret_cast<TooltipWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (!window) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
    case WM_PAINT:
        window->render();
        return 0;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
