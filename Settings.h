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

    // Mixer window settings
    int getMixerWindowX() const { return m_mixerWindowX; }
    int getMixerWindowY() const { return m_mixerWindowY; }
    int getMixerWindowWidth() const { return m_mixerWindowWidth; }
    int getMixerWindowHeight() const { return m_mixerWindowHeight; }
    bool getMixerWindowVisible() const { return m_mixerWindowVisible; }

    void setMixerWindowX(int x) { m_mixerWindowX = x; }
    void setMixerWindowY(int y) { m_mixerWindowY = y; }
    void setMixerWindowWidth(int width) { m_mixerWindowWidth = width; }
    void setMixerWindowHeight(int height) { m_mixerWindowHeight = height; }
    void setMixerWindowVisible(bool visible) { m_mixerWindowVisible = visible; }

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

    // Mixer window settings
    int m_mixerWindowX = 100;
    int m_mixerWindowY = 100;
    int m_mixerWindowWidth = 800;
    int m_mixerWindowHeight = 600;
    bool m_mixerWindowVisible = false;

    // Timeline settings
    double m_pixelsPerSecond = 100.0;
    bool m_followPlayhead = true;
    bool m_showGrid = true;
    bool m_snapToGrid = true;
    double m_bpm = 120.0;

    // Last opened project
    std::wstring m_lastProjectPath;
};
