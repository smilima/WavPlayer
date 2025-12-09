#pragma once
#include "D2DWindow.h"
#include "TransportBar.h"
#include "TimelineView.h"
#include "AudioEngine.h"
#include <memory>

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool create(const wchar_t* title, int width, int height);
    HWND getHWND() const { return m_hwnd; }

    // File operations
    bool loadAudioFile(const std::wstring& filename);
    
    // Playback
    void play();
    void pause();
    void stop();
    void togglePlayPause();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void onResize(int width, int height);
    void onCommand(int id);
    void onDropFiles(HDROP hDrop);
    void setupMenus();
    void updatePlaybackPosition();
    
    HWND m_hwnd = nullptr;
    UINT_PTR m_timerId = 0;
    
    std::unique_ptr<TransportBar> m_transportBar;
    std::unique_ptr<TimelineView> m_timelineView;
    std::unique_ptr<AudioEngine> m_audioEngine;
    
    // Current project state
    std::vector<std::shared_ptr<AudioClip>> m_loadedClips;
    double m_bpm = 120.0;
};
