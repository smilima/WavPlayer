#pragma once
#include "D2DWindow.h"
#include "TransportBar.h"
#include "TimelineView.h"
#include "AudioEngine.h"
#include "Project.h"
#include <memory>

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool create(const wchar_t* title, int width, int height);
    HWND getHWND() const { return m_hwnd; }

    // Project operations
    void newProject();
    bool openProject();
    bool saveProject();
    bool saveProjectAs();
    bool closeProject();
    
    // File operations
    bool importAudioFile();
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
    void onClose();
    void setupMenus();
    void updatePlaybackPosition();
    void updateWindowTitle();
    void syncProjectToUI();
    void syncUIToProject();
    bool promptSaveIfModified();  // Returns false if user cancels
    
    HWND m_hwnd = nullptr;
    UINT_PTR m_timerId = 0;
    
    std::unique_ptr<TransportBar> m_transportBar;
    std::unique_ptr<TimelineView> m_timelineView;
    std::unique_ptr<AudioEngine> m_audioEngine;
    std::unique_ptr<Project> m_project;
};
