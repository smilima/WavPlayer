#include "Settings.h"
#include <ShlObj.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "Shell32.lib")

Settings::Settings() {
    m_settingsPath = getSettingsFilePath();
}

std::wstring Settings::getSettingsFilePath() {
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath))) {
        std::wstring path = appDataPath;
        path += L"\\WavPlayer";

        // Create directory if it doesn't exist
        CreateDirectory(path.c_str(), nullptr);

        path += L"\\settings.ini";
        return path;
    }

    // Fallback to current directory
    return L".\\settings.ini";
}

void Settings::load() {
    // Window settings
    m_windowX = readInt(L"Window", L"X", m_windowX);
    m_windowY = readInt(L"Window", L"Y", m_windowY);
    m_windowWidth = readInt(L"Window", L"Width", m_windowWidth);
    m_windowHeight = readInt(L"Window", L"Height", m_windowHeight);
    m_windowMaximized = readBool(L"Window", L"Maximized", m_windowMaximized);

    // Mixer window settings
    m_mixerWindowX = readInt(L"MixerWindow", L"X", m_mixerWindowX);
    m_mixerWindowY = readInt(L"MixerWindow", L"Y", m_mixerWindowY);
    m_mixerWindowWidth = readInt(L"MixerWindow", L"Width", m_mixerWindowWidth);
    m_mixerWindowHeight = readInt(L"MixerWindow", L"Height", m_mixerWindowHeight);

    // Timeline settings
    m_pixelsPerSecond = readDouble(L"Timeline", L"PixelsPerSecond", m_pixelsPerSecond);
    m_followPlayhead = readBool(L"Timeline", L"FollowPlayhead", m_followPlayhead);
    m_showGrid = readBool(L"Timeline", L"ShowGrid", m_showGrid);
    m_snapToGrid = readBool(L"Timeline", L"SnapToGrid", m_snapToGrid);
    m_bpm = readDouble(L"Timeline", L"BPM", m_bpm);

    // Last project
    m_lastProjectPath = readString(L"General", L"LastProjectPath", m_lastProjectPath);
}

void Settings::save() {
    // Window settings
    writeInt(L"Window", L"X", m_windowX);
    writeInt(L"Window", L"Y", m_windowY);
    writeInt(L"Window", L"Width", m_windowWidth);
    writeInt(L"Window", L"Height", m_windowHeight);
    writeBool(L"Window", L"Maximized", m_windowMaximized);

    // Mixer window settings
    writeInt(L"MixerWindow", L"X", m_mixerWindowX);
    writeInt(L"MixerWindow", L"Y", m_mixerWindowY);
    writeInt(L"MixerWindow", L"Width", m_mixerWindowWidth);
    writeInt(L"MixerWindow", L"Height", m_mixerWindowHeight);

    // Timeline settings
    writeDouble(L"Timeline", L"PixelsPerSecond", m_pixelsPerSecond);
    writeBool(L"Timeline", L"FollowPlayhead", m_followPlayhead);
    writeBool(L"Timeline", L"ShowGrid", m_showGrid);
    writeBool(L"Timeline", L"SnapToGrid", m_snapToGrid);
    writeDouble(L"Timeline", L"BPM", m_bpm);

    // Last project
    writeString(L"General", L"LastProjectPath", m_lastProjectPath);
}

int Settings::readInt(const wchar_t* section, const wchar_t* key, int defaultValue) {
    return GetPrivateProfileInt(section, key, defaultValue, m_settingsPath.c_str());
}

double Settings::readDouble(const wchar_t* section, const wchar_t* key, double defaultValue) {
    wchar_t buffer[256];
    std::wostringstream defaultStr;
    defaultStr << std::fixed << std::setprecision(6) << defaultValue;

    GetPrivateProfileString(section, key, defaultStr.str().c_str(), buffer, 256, m_settingsPath.c_str());

    try {
        return std::stod(buffer);
    } catch (...) {
        return defaultValue;
    }
}

bool Settings::readBool(const wchar_t* section, const wchar_t* key, bool defaultValue) {
    return GetPrivateProfileInt(section, key, defaultValue ? 1 : 0, m_settingsPath.c_str()) != 0;
}

std::wstring Settings::readString(const wchar_t* section, const wchar_t* key, const std::wstring& defaultValue) {
    wchar_t buffer[1024];
    GetPrivateProfileString(section, key, defaultValue.c_str(), buffer, 1024, m_settingsPath.c_str());
    return buffer;
}

void Settings::writeInt(const wchar_t* section, const wchar_t* key, int value) {
    std::wostringstream ss;
    ss << value;
    WritePrivateProfileString(section, key, ss.str().c_str(), m_settingsPath.c_str());
}

void Settings::writeDouble(const wchar_t* section, const wchar_t* key, double value) {
    std::wostringstream ss;
    ss << std::fixed << std::setprecision(6) << value;
    WritePrivateProfileString(section, key, ss.str().c_str(), m_settingsPath.c_str());
}

void Settings::writeBool(const wchar_t* section, const wchar_t* key, bool value) {
    WritePrivateProfileString(section, key, value ? L"1" : L"0", m_settingsPath.c_str());
}

void Settings::writeString(const wchar_t* section, const wchar_t* key, const std::wstring& value) {
    WritePrivateProfileString(section, key, value.c_str(), m_settingsPath.c_str());
}
