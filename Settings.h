#pragma once
#include <Windows.h>
#include <string>

class Settings {
public:
    Settings();
    ~Settings() = default;

    // Load settings from file
    void load();

    // Save settings to file
    void save();

    // Window settings
    int getWindowX() const { return m_windowX; }
    int getWindowY() const { return m_windowY; }
    int getWindowWidth() const { return m_windowWidth; }
    int getWindowHeight() const { return m_windowHeight; }
    bool getWindowMaximized() const { return m_windowMaximized; }

    void setWindowX(int x) { m_windowX = x; }
    void setWindowY(int y) { m_windowY = y; }
    void setWindowWidth(int width) { m_windowWidth = width; }
    void setWindowHeight(int height) { m_windowHeight = height; }
    void setWindowMaximized(bool maximized) { m_windowMaximized = maximized; }

    // Timeline settings
    double getPixelsPerSecond() const { return m_pixelsPerSecond; }
    bool getFollowPlayhead() const { return m_followPlayhead; }
    bool getShowGrid() const { return m_showGrid; }
    bool getSnapToGrid() const { return m_snapToGrid; }
    double getBPM() const { return m_bpm; }

    void setPixelsPerSecond(double pps) { m_pixelsPerSecond = pps; }
    void setFollowPlayhead(bool follow) { m_followPlayhead = follow; }
    void setShowGrid(bool show) { m_showGrid = show; }
    void setSnapToGrid(bool snap) { m_snapToGrid = snap; }
    void setBPM(double bpm) { m_bpm = bpm; }

    // Last opened project
    std::wstring getLastProjectPath() const { return m_lastProjectPath; }
    void setLastProjectPath(const std::wstring& path) { m_lastProjectPath = path; }

private:
    std::wstring getSettingsFilePath();
    int readInt(const wchar_t* section, const wchar_t* key, int defaultValue);
    double readDouble(const wchar_t* section, const wchar_t* key, double defaultValue);
    bool readBool(const wchar_t* section, const wchar_t* key, bool defaultValue);
    std::wstring readString(const wchar_t* section, const wchar_t* key, const std::wstring& defaultValue);

    void writeInt(const wchar_t* section, const wchar_t* key, int value);
    void writeDouble(const wchar_t* section, const wchar_t* key, double value);
    void writeBool(const wchar_t* section, const wchar_t* key, bool value);
    void writeString(const wchar_t* section, const wchar_t* key, const std::wstring& value);

    std::wstring m_settingsPath;

    // Window settings
    int m_windowX = 100;
    int m_windowY = 100;
    int m_windowWidth = 1280;
    int m_windowHeight = 720;
    bool m_windowMaximized = false;

    // Timeline settings
    double m_pixelsPerSecond = 100.0;
    bool m_followPlayhead = true;
    bool m_showGrid = true;
    bool m_snapToGrid = true;
    double m_bpm = 120.0;

    // Last opened project
    std::wstring m_lastProjectPath;
};
