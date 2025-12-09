#include "D2DWindow.h"
#include "Application.h"
#include <windowsx.h>  // For GET_X_LPARAM, GET_Y_LPARAM

int D2DWindow::s_windowCount = 0;

D2DWindow::~D2DWindow() {
    discardDeviceResources();
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
}

bool D2DWindow::create(HWND parent, int x, int y, int width, int height,
                        const wchar_t* className) {
    // Generate unique class name if not provided
    std::wstring classNameStr;
    if (!className) {
        classNameStr = L"D2DWindow_" + std::to_wstring(s_windowCount++);
        className = classNameStr.c_str();
    }

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;  // CS_DBLCLKS for double-click support
    wc.lpfnWndProc = WndProc;
    wc.hInstance = Application::getInstance().getHInstance();
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = className;
    
    RegisterClassEx(&wc);

    DWORD style = parent ? (WS_CHILD | WS_VISIBLE) : (WS_OVERLAPPEDWINDOW);
    
    m_hwnd = CreateWindowEx(
        0,
        className,
        L"",
        style,
        x, y, width, height,
        parent,
        nullptr,
        Application::getInstance().getHInstance(),
        this
    );

    if (!m_hwnd) return false;

    // Initialize DPI scale and get dimensions in DIPs
    updateDpiScale();

    return true;
}

void D2DWindow::updateDpiScale() {
    // Get DPI for this window
    HDC hdc = GetDC(m_hwnd);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(m_hwnd, hdc);
    
    m_dpiScaleX = dpiX / 96.0f;
    m_dpiScaleY = dpiY / 96.0f;
    
    // Get client rect in physical pixels, convert to DIPs
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    m_width = static_cast<int>((rc.right - rc.left) / m_dpiScaleX);
    m_height = static_cast<int>((rc.bottom - rc.top) / m_dpiScaleY);
}

void D2DWindow::invalidate() {
    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

bool D2DWindow::createDeviceResources() {
    if (m_renderTarget) return true;

    auto factory = Application::getInstance().getD2DFactory();
    auto dwriteFactory = Application::getInstance().getDWriteFactory();

    RECT rc;
    GetClientRect(m_hwnd, &rc);

    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    HRESULT hr = factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(m_hwnd, size),
        &m_renderTarget
    );

    if (FAILED(hr)) return false;

    hr = m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &m_brush);
    if (FAILED(hr)) return false;

    // Create default text format
    hr = dwriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"en-us",
        &m_textFormat
    );

    if (SUCCEEDED(hr)) {
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // Create small text format
    hr = dwriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        10.0f,
        L"en-us",
        &m_textFormatSmall
    );

    return true;
}

void D2DWindow::discardDeviceResources() {
    if (m_textFormatSmall) { m_textFormatSmall->Release(); m_textFormatSmall = nullptr; }
    if (m_textFormat) { m_textFormat->Release(); m_textFormat = nullptr; }
    if (m_brush) { m_brush->Release(); m_brush = nullptr; }
    if (m_renderTarget) { m_renderTarget->Release(); m_renderTarget = nullptr; }
}

void D2DWindow::drawText(const std::wstring& text, float x, float y,
                          const Color& color, float maxWidth, float maxHeight) {
    if (!m_renderTarget || !m_textFormat) return;
    
    m_brush->SetColor(color.toD2D());
    
    if (maxWidth <= 0) maxWidth = static_cast<float>(m_width) - x;
    if (maxHeight <= 0) maxHeight = 24.0f;
    
    D2D1_RECT_F rect = D2D1::RectF(x, y, x + maxWidth, y + maxHeight);
    m_renderTarget->DrawText(text.c_str(), static_cast<UINT32>(text.length()),
                              m_textFormat, rect, m_brush);
}

void D2DWindow::fillRect(float x, float y, float w, float h, const Color& color) {
    if (!m_renderTarget) return;
    m_brush->SetColor(color.toD2D());
    m_renderTarget->FillRectangle(D2D1::RectF(x, y, x + w, y + h), m_brush);
}

void D2DWindow::drawRect(float x, float y, float w, float h, 
                          const Color& color, float strokeWidth) {
    if (!m_renderTarget) return;
    m_brush->SetColor(color.toD2D());
    m_renderTarget->DrawRectangle(D2D1::RectF(x, y, x + w, y + h), m_brush, strokeWidth);
}

void D2DWindow::drawLine(float x1, float y1, float x2, float y2,
                          const Color& color, float strokeWidth) {
    if (!m_renderTarget) return;
    m_brush->SetColor(color.toD2D());
    m_renderTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), 
                              m_brush, strokeWidth);
}

LRESULT CALLBACK D2DWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    D2DWindow* window = nullptr;

    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = static_cast<D2DWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->m_hwnd = hwnd;
    }
    else {
        window = reinterpret_cast<D2DWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (!window) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
    case WM_SIZE:
        {
            // Update DPI scale (in case of DPI change during move)
            window->updateDpiScale();
            
            // Resize render target if it exists
            if (window->m_renderTarget) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
                window->m_renderTarget->Resize(size);
            }
            
            // Notify subclass with DIP dimensions
            window->onResize(window->m_width, window->m_height);
            window->invalidate();
        }
        return 0;

    case WM_PAINT:
    case WM_DISPLAYCHANGE:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            
            if (window->createDeviceResources()) {
                window->m_renderTarget->BeginDraw();
                window->onRender(window->m_renderTarget);
                
                HRESULT hr = window->m_renderTarget->EndDraw();
                if (hr == D2DERR_RECREATE_TARGET) {
                    window->discardDeviceResources();
                }
            }
            
            EndPaint(hwnd, &ps);
        }
        return 0;

    case WM_LBUTTONDOWN:
        {
            int dipX = static_cast<int>(window->pixelsToDipsX(GET_X_LPARAM(lParam)));
            int dipY = static_cast<int>(window->pixelsToDipsY(GET_Y_LPARAM(lParam)));
            window->onMouseDown(dipX, dipY, 0);
            SetCapture(hwnd);
        }
        return 0;

    case WM_LBUTTONDBLCLK:
        {
            int dipX = static_cast<int>(window->pixelsToDipsX(GET_X_LPARAM(lParam)));
            int dipY = static_cast<int>(window->pixelsToDipsY(GET_Y_LPARAM(lParam)));
            window->onDoubleClick(dipX, dipY, 0);
        }
        return 0;

    case WM_RBUTTONDOWN:
        {
            int dipX = static_cast<int>(window->pixelsToDipsX(GET_X_LPARAM(lParam)));
            int dipY = static_cast<int>(window->pixelsToDipsY(GET_Y_LPARAM(lParam)));
            window->onMouseDown(dipX, dipY, 1);
        }
        return 0;

    case WM_MBUTTONDOWN:
        {
            int dipX = static_cast<int>(window->pixelsToDipsX(GET_X_LPARAM(lParam)));
            int dipY = static_cast<int>(window->pixelsToDipsY(GET_Y_LPARAM(lParam)));
            window->onMouseDown(dipX, dipY, 2);
        }
        return 0;

    case WM_LBUTTONUP:
        {
            int dipX = static_cast<int>(window->pixelsToDipsX(GET_X_LPARAM(lParam)));
            int dipY = static_cast<int>(window->pixelsToDipsY(GET_Y_LPARAM(lParam)));
            window->onMouseUp(dipX, dipY, 0);
            ReleaseCapture();
        }
        return 0;

    case WM_RBUTTONUP:
        {
            int dipX = static_cast<int>(window->pixelsToDipsX(GET_X_LPARAM(lParam)));
            int dipY = static_cast<int>(window->pixelsToDipsY(GET_Y_LPARAM(lParam)));
            window->onMouseUp(dipX, dipY, 1);
        }
        return 0;

    case WM_MBUTTONUP:
        {
            int dipX = static_cast<int>(window->pixelsToDipsX(GET_X_LPARAM(lParam)));
            int dipY = static_cast<int>(window->pixelsToDipsY(GET_Y_LPARAM(lParam)));
            window->onMouseUp(dipX, dipY, 2);
        }
        return 0;

    case WM_MOUSEMOVE:
        {
            int dipX = static_cast<int>(window->pixelsToDipsX(GET_X_LPARAM(lParam)));
            int dipY = static_cast<int>(window->pixelsToDipsY(GET_Y_LPARAM(lParam)));
            window->onMouseMove(dipX, dipY);
        }
        return 0;

    case WM_MOUSEWHEEL:
        {
            // WM_MOUSEWHEEL gives screen coordinates, convert to client then to DIPs
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            int dipX = static_cast<int>(window->pixelsToDipsX(pt.x));
            int dipY = static_cast<int>(window->pixelsToDipsY(pt.y));
            window->onMouseWheel(dipX, dipY, GET_WHEEL_DELTA_WPARAM(wParam));
        }
        return 0;

    case WM_KEYDOWN:
        window->onKeyDown(static_cast<int>(wParam));
        return 0;

    case WM_KEYUP:
        window->onKeyUp(static_cast<int>(wParam));
        return 0;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
