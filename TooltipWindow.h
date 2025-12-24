#pragma once
#include <Windows.h>
#include <string>

class TooltipWindow {
public:
    TooltipWindow();
    ~TooltipWindow();

    bool create(HWND parent);
    void show(const std::wstring& text, int screenX, int screenY, bool positionAbove = true);
    void hide();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void render();
    void updateSize(const std::wstring& text);

    HWND m_hwnd = nullptr;
    HWND m_parent = nullptr;
    std::wstring m_text;
    int m_width = 0;
    int m_height = 0;

    static bool s_classRegistered;
};
