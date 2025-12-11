#pragma once
#include "D2DWindow.h"
#include "TransportBar.h"
#include "TimelineView.h"
#include "AudioEngine.h"
#include "Project.h"
#include <memory>
#include <filesystem>

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
    
    // Recording
    void startRecording();
    void stopRecording();
    void toggleRecording();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void onResize(int width, int height);
    void onCommand(int id);
    void onDropFiles(HDROP hDrop);
    void onClose();
    void setupMenus() const;
    void updatePlaybackPosition();
    void updateWindowTitle();
    void syncProjectToUI();
    void syncUIToProject();
    bool promptSaveIfModified();  // Returns false if user cancels
    void onRecordingComplete(std::shared_ptr<AudioClip> clip);
    bool saveRecordedClip(std::shared_ptr<AudioClip> clip);  // Keep for manual save if needed
    bool autoSaveRecordedClip(std::shared_ptr<AudioClip> clip);  // Auto-save without dialog
    
    HWND m_hwnd = nullptr;
    UINT_PTR m_timerId = 0;
    
    std::unique_ptr<TransportBar> m_transportBar;
    std::unique_ptr<TimelineView> m_timelineView;
    std::unique_ptr<AudioEngine> m_audioEngine;
    std::unique_ptr<Project> m_project;
    
    int m_recordingCount = 0;  // Counter for auto-naming recordings
    int m_recordingTrackIndex = -1;  // Track number (1-based) for recording filename
    std::shared_ptr<Track> m_recordingTrack;  // Track that was armed when recording started
    double m_recordingStartPosition = 0.0;  // Playhead position when recording started (punch-in point)
};
