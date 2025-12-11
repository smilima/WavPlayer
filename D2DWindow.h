#pragma once
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>

// Color helper
struct Color {
    float r, g, b, a;
    
    Color(float r = 0, float g = 0, float b = 0, float a = 1) 
        : r(r), g(g), b(b), a(a) {}
    
    Color(uint32_t argb) {
        a = ((argb >> 24) & 0xFF) / 255.0f;
        r = ((argb >> 16) & 0xFF) / 255.0f;
        g = ((argb >> 8) & 0xFF) / 255.0f;
        b = (argb & 0xFF) / 255.0f;
    }
    
    D2D1_COLOR_F toD2D() const {
        return D2D1::ColorF(r, g, b, a);
    }
    
    static Color fromRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }
};

// Common DAW colors
namespace DAWColors {
    const Color Background(0.12f, 0.12f, 0.14f);
    const Color TrackBackground(0.16f, 0.16f, 0.18f);
    const Color TrackHeader(0.20f, 0.20f, 0.22f);
    const Color Timeline(0.10f, 0.10f, 0.12f);
    const Color TimelineText(0.6f, 0.6f, 0.6f);
    const Color GridLine(0.25f, 0.25f, 0.28f);
    const Color GridLineMajor(0.35f, 0.35f, 0.38f);
    const Color Playhead(1.0f, 0.3f, 0.3f);
    const Color Selection(0.3f, 0.5f, 0.8f, 0.3f);
    const Color Waveform(0.4f, 0.7f, 0.9f);
    const Color WaveformPeak(0.5f, 0.8f, 1.0f);
    const Color ButtonNormal(0.25f, 0.25f, 0.28f);
    const Color ButtonHover(0.35f, 0.35f, 0.38f);
    const Color ButtonPressed(0.2f, 0.2f, 0.22f);
    const Color TextPrimary(0.9f, 0.9f, 0.9f);
    const Color TextSecondary(0.6f, 0.6f, 0.6f);
    const Color Transport(0.14f, 0.14f, 0.16f);
}

class D2DWindow {
public:
    virtual ~D2DWindow();

    bool create(HWND parent, int x, int y, int width, int height, 
                const wchar_t* className = nullptr);
    
    HWND getHWND() const { return m_hwnd; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
    void invalidate();

protected:
    virtual void onRender(ID2D1RenderTarget* rt) = 0;
    virtual void onResize(int width, int height) {}
    virtual void onMouseDown(int x, int y, int button) {}
    virtual void onMouseUp(int x, int y, int button) {}
    virtual void onMouseMove(int x, int y) {}
    virtual void onMouseWheel(int x, int y, int delta) {}
    virtual void onDoubleClick(int x, int y, int button) {}
    virtual void onKeyDown(int vkey) {}
    virtual void onKeyUp(int vkey) {}
    virtual void onHScroll(int request, int pos) {}

    bool createDeviceResources();
    void discardDeviceResources();
    
    ID2D1HwndRenderTarget* getRenderTarget() { return m_renderTarget; }
    ID2D1SolidColorBrush* getBrush() { return m_brush; }
    IDWriteTextFormat* getTextFormat() { return m_textFormat; }

    // Helper methods
    void drawText(const std::wstring& text, float x, float y, 
                  const Color& color, float maxWidth = 0, float maxHeight = 0);
    void fillRect(float x, float y, float w, float h, const Color& color);
    void drawRect(float x, float y, float w, float h, const Color& color, float strokeWidth = 1.0f);
    void drawLine(float x1, float y1, float x2, float y2, const Color& color, float strokeWidth = 1.0f);

    // DPI helpers - convert physical pixels to DIPs (D2D coordinate space)
    float pixelsToDipsX(int pixels) const { return pixels / m_dpiScaleX; }
    float pixelsToDipsY(int pixels) const { return pixels / m_dpiScaleY; }
    float dipsToPixelsX(float dips) const { return dips * m_dpiScaleX; }
    float dipsToPixelsY(float dips) const { return dips * m_dpiScaleY; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static int s_windowCount;

    void updateDpiScale();

    HWND m_hwnd = nullptr;
    int m_width = 0;   // Width in DIPs
    int m_height = 0;  // Height in DIPs
    
    float m_dpiScaleX = 1.0f;  // DPI / 96.0
    float m_dpiScaleY = 1.0f;
    
    ID2D1HwndRenderTarget* m_renderTarget = nullptr;
    ID2D1SolidColorBrush* m_brush = nullptr;
    IDWriteTextFormat* m_textFormat = nullptr;
    IDWriteTextFormat* m_textFormatSmall = nullptr;
};
