#pragma once
#include "D2DWindow.h"
#include "TransportBar.h"
#include "TimelineView.h"
#include "AudioEngine.h"
#include "Project.h"
#include "SpectrumWindow.h"
#include "MixerWindow.h"
#include "Settings.h"
#include <memory>
#include <filesystem>
#include <string>
#include <vector>

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
    void stop(bool resetPlayhead = false);
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
    bool promptSaveIfModified();
    void onRecordingComplete(std::shared_ptr<AudioClip> clip);
    bool saveRecordedClip(std::shared_ptr<AudioClip> clip);
    bool autoSaveRecordedClip(std::shared_ptr<AudioClip> clip);

    bool registerWindowClass() const;
    bool createNativeWindow(const wchar_t* title, int width, int height);
    bool initializeAudioEngine();
    void createChildViews();
    void configureTimelineCallbacks();
    void configureTransportCallbacks();
    void configureAudioCallbacks();
    void startPlaybackTimer();
    void stopPlaybackTimer();
    void ensureAudioEngineTracks();
    double calculateProjectDuration() const;
    void applyProjectDuration(double duration);
    void refreshProjectDuration();
    void resetPlaybackToStart();
    void addDefaultTrack();
    void resetTrackNumbering();
    void markProjectModified();
    void handleTrackAdd();
    void handleTrackDelete();
    bool shouldDeleteTrackAudio(const std::shared_ptr<Track>& track,
        std::vector<std::wstring>& filesToDelete) const;
    void deleteAudioFiles(const std::vector<std::wstring>& filesToDelete);
    void toggleFollowPlayhead();
    void updateFollowPlayheadMenu();
    void loadSettings();
    void saveSettings();
    void saveWindowPosition();
    void saveMixerWindowPosition();

	void showAboutDialog() const;

    HWND m_hwnd = nullptr;
    UINT_PTR m_timerId = 0;
    Settings m_settings;
    
    std::unique_ptr<TransportBar> m_transportBar;
    std::unique_ptr<TimelineView> m_timelineView;
    std::unique_ptr<AudioEngine> m_audioEngine;
    std::unique_ptr<Project> m_project;
    std::unique_ptr<SpectrumWindow> m_spectrumWindow;
    std::unique_ptr<MixerWindow> m_mixerWindow;

    int m_nextTrackNumber = 2;
    int m_recordingCount = 0;
    int m_recordingTrackIndex = -1;
    std::shared_ptr<Track> m_recordingTrack;
    double m_recordingStartPosition = 0.0;
};
