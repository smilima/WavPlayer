#include "MainWindow.h"
#include "Application.h"
#include "resource.h"

#include <Shlwapi.h>
#include <commdlg.h>
#include <shellapi.h>

#include <array>
#include <algorithm>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Shell32.lib")

namespace {
constexpr wchar_t WINDOW_CLASS_NAME[] = L"DAWMainWindow";
constexpr UINT_PTR TIMER_PLAYBACK = 1;
constexpr UINT PLAYBACK_TIMER_INTERVAL_MS = 33;
constexpr int TRANSPORT_HEIGHT = 50;
constexpr double MAX_RECORDING_SECONDS = 3600.0;
constexpr std::array<uint32_t, 5> TRACK_COLORS = {
    0xFF4A90D9, 0xFF5CB85C, 0xFFD9534F, 0xFFF0AD4E, 0xFF9B59B6
};
}

// Menu IDs
enum MenuID {
    ID_FILE_NEW = 1001,
    ID_FILE_OPEN,
    ID_FILE_SAVE,
    ID_FILE_SAVE_AS,
    ID_FILE_CLOSE,
    ID_FILE_IMPORT_AUDIO,
    ID_FILE_EXIT,
    ID_EDIT_UNDO,
    ID_EDIT_REDO,
    ID_EDIT_CUT,
    ID_EDIT_COPY,
    ID_EDIT_PASTE,
    ID_EDIT_DELETE,
    ID_TRANSPORT_PLAY,
    ID_TRANSPORT_STOP,
    ID_TRANSPORT_REWIND,
    ID_TRANSPORT_RECORD,
    ID_TRACK_ADD,
    ID_TRACK_DELETE,
    ID_VIEW_ZOOM_IN,
    ID_VIEW_ZOOM_OUT,
    ID_VIEW_ZOOM_FIT,
    ID_HELP_ABOUT 
};

MainWindow::MainWindow() = default;

MainWindow::~MainWindow() {
    stopPlaybackTimer();
}

bool MainWindow::create(const wchar_t* title, int width, int height) {
    if (!registerWindowClass()) {
        return false;
    }

    if (!createNativeWindow(title, width, height)) {
        return false;
    }

    setupMenus();

    if (!initializeAudioEngine()) {
        MessageBox(m_hwnd, L"Failed to initialize audio engine", L"Error", MB_OK);
    }

    m_project = std::make_unique<Project>();

    createChildViews();
    configureTimelineCallbacks();
    configureTransportCallbacks();
    configureAudioCallbacks();
    syncProjectToUI();

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    startPlaybackTimer();
    updateWindowTitle();
    return true;
}

bool MainWindow::registerWindowClass() const {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = Application::getInstance().getHInstance();
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_WAVPLAYER));
    wc.hIconSm = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_WAVPLAYER));

    ATOM atom = RegisterClassEx(&wc);
    if (!atom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    return true;
}

bool MainWindow::createNativeWindow(const wchar_t* title, int width, int height) {
    m_hwnd = CreateWindowEx(
        WS_EX_ACCEPTFILES,
        WINDOW_CLASS_NAME,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        Application::getInstance().getHInstance(),
        this
    );

    return m_hwnd != nullptr;
}

bool MainWindow::initializeAudioEngine() {
    m_audioEngine = std::make_unique<AudioEngine>();
    return m_audioEngine->initialize(44100, 2);
}

void MainWindow::createChildViews() {
    RECT rc{};
    GetClientRect(m_hwnd, &rc);
    const int clientWidth = rc.right - rc.left;
    const int clientHeight = rc.bottom - rc.top;
    const int timelineHeight = clientHeight - TRANSPORT_HEIGHT;

    m_timelineView = std::make_unique<TimelineView>();
    m_timelineView->create(m_hwnd, 0, 0, clientWidth, timelineHeight);

    m_transportBar = std::make_unique<TransportBar>();
    m_transportBar->create(m_hwnd, 0, timelineHeight, clientWidth, TRANSPORT_HEIGHT);
}

void MainWindow::configureTimelineCallbacks() {
    m_timelineView->setPlayheadCallback([this](double time) {
        if (m_audioEngine) {
            m_audioEngine->setPosition(time);
        }
        if (m_transportBar) {
            m_transportBar->setPosition(time);
        }
    });

    m_timelineView->setRegionChangedCallback([this]() {
        refreshProjectDuration();
        markProjectModified();
    });

    m_timelineView->setTrackDeleteCallback([this]() {
        handleTrackDelete();
    });
}

void MainWindow::configureTransportCallbacks() {
    m_transportBar->setPlayCallback([this]() { play(); });
    m_transportBar->setPauseCallback([this]() { pause(); });
    m_transportBar->setStopCallback([this]() { stop(); });
    m_transportBar->setRewindCallback([this]() { resetPlaybackToStart(); });
    m_transportBar->setRecordCallback([this]() { toggleRecording(); });
}

void MainWindow::configureAudioCallbacks() {
    if (m_audioEngine) {
        m_audioEngine->setRecordingCallback([this](std::shared_ptr<AudioClip> clip) {
            onRecordingComplete(std::move(clip));
        });
    }
}

void MainWindow::startPlaybackTimer() {
    if (!m_timerId) {
        m_timerId = SetTimer(m_hwnd, TIMER_PLAYBACK, PLAYBACK_TIMER_INTERVAL_MS, nullptr);
    }
}

void MainWindow::stopPlaybackTimer() {
    if (m_timerId) {
        KillTimer(m_hwnd, m_timerId);
        m_timerId = 0;
    }
}

void MainWindow::ensureAudioEngineTracks() {
    if (m_audioEngine && m_project) {
        m_audioEngine->setTracks(&m_project->getTracks());
    }
}

double MainWindow::calculateProjectDuration() const {
    if (!m_project) {
        return 0.0;
    }

    double maxDuration = 0.0;
    for (const auto& track : m_project->getTracks()) {
        for (const auto& region : track->getRegions()) {
            maxDuration = std::max(maxDuration, region.endTime());
        }
    }
    return maxDuration;
}

void MainWindow::applyProjectDuration(double duration) {
    if (m_transportBar) {
        m_transportBar->setDuration(duration);
    }
    if (m_audioEngine) {
        m_audioEngine->setDuration(duration);
    }
}

void MainWindow::refreshProjectDuration() {
    ensureAudioEngineTracks();
    const double duration = calculateProjectDuration();
    applyProjectDuration(duration);

    if (m_timelineView) {
        m_timelineView->setTimelineDuration(duration);
    }

    if (m_transportBar && m_project) {
        m_transportBar->setHasAudioLoaded(m_project->hasAudioLoaded());
    }
}

void MainWindow::resetPlaybackToStart() {
    if (m_audioEngine) {
        m_audioEngine->setPosition(0);
    }
    if (m_timelineView) {
        m_timelineView->setPlayheadPosition(0);
    }
    if (m_transportBar) {
        m_transportBar->setPosition(0);
    }
}

void MainWindow::addDefaultTrack() {
    auto track = std::make_shared<Track>(L"Track 1");
    track->setColor(0xFF4A90D9);
    m_project->addTrack(track);
}

void MainWindow::resetTrackNumbering() {
    if (!m_project) {
        m_nextTrackNumber = 1;
        return;
    }
    m_nextTrackNumber = static_cast<int>(m_project->getTracks().size()) + 1;
}

void MainWindow::markProjectModified() {
    if (m_project) {
        m_project->setModified(true);
        updateWindowTitle();
    }
}

void MainWindow::handleTrackAdd() {
    const int trackNumber = m_nextTrackNumber++;
    const size_t colorIndex =
        static_cast<size_t>((trackNumber - 2) % TRACK_COLORS.size());

    auto track = std::make_shared<Track>(L"Track " + std::to_wstring(trackNumber));
    track->setColor(TRACK_COLORS[colorIndex]);
    track->setVisible(true);

    m_project->addTrack(track);
    m_timelineView->addTrack(track);
    markProjectModified();
    m_timelineView->invalidate();
}

void MainWindow::handleTrackDelete() {
    const int index = m_timelineView->getSelectedTrackIndex();
    if (index < 0 || index >= static_cast<int>(m_project->getTracks().size())) {
        MessageBox(m_hwnd, L"Please select a track to delete.", L"No Selection", MB_OK);
        return;
    }

    auto track = m_project->getTracks()[index];
    std::vector<std::wstring> filesToDelete;
    if (!shouldDeleteTrackAudio(track, filesToDelete)) {
        return;
    }

    m_project->removeTrack(index);
    m_timelineView->removeTrack(index);
    m_timelineView->setSelectedTrackIndex(-1);

    deleteAudioFiles(filesToDelete);
    markProjectModified();
    refreshProjectDuration();
    m_timelineView->invalidate();
    resetTrackNumbering();
}

bool MainWindow::shouldDeleteTrackAudio(
    const std::shared_ptr<Track>& track,
    std::vector<std::wstring>& filesToDelete) const {

    if (!track) {
        return true;
    }

    const bool hasAudioFiles = std::any_of(
        track->getRegions().begin(),
        track->getRegions().end(),
        [](const TrackRegion& region) { return region.clip != nullptr; });

    if (!hasAudioFiles) {
        return true;
    }

    const int result = MessageBox(
        m_hwnd,
        L"Do you want to permanently delete the audio files associated with this track?\n\n"
        L"Yes = Delete track AND audio files\n"
        L"No = Delete track only (keep audio files)\n"
        L"Cancel = Don't delete anything",
        L"Delete Track",
        MB_YESNOCANCEL | MB_ICONQUESTION
    );

    if (result == IDCANCEL) {
        return false;
    }

    if (result == IDYES) {
        const auto& cache = m_project->getClipCache();
        for (const auto& region : track->getRegions()) {
            if (!region.clip) {
                continue;
            }

            const auto it = std::find_if(
                cache.begin(),
                cache.end(),
                [&](const auto& entry) { return entry.second == region.clip; });

            if (it != cache.end() &&
                std::find(filesToDelete.begin(), filesToDelete.end(), it->first) == filesToDelete.end()) {
                filesToDelete.push_back(it->first);
            }
        }
    }

    return true;
}

void MainWindow::deleteAudioFiles(const std::vector<std::wstring>& filesToDelete) {
    for (const auto& filepath : filesToDelete) {
        m_project->removeClipFromCache(filepath);
        if (!DeleteFile(filepath.c_str())) {
            const std::wstring message = L"Failed to delete file:\n" + filepath;
            MessageBox(m_hwnd, message.c_str(), L"Warning", MB_OK | MB_ICONWARNING);
        }
    }
}

// Dialog procedure for About dialog
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        {
            // Set larger, bold font for title
            HWND hTitle = GetDlgItem(hDlg, IDC_STATIC_TITLE);
            if (hTitle) {
                HFONT hFont = CreateFont(
                    28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                    L"Segoe UI"
                );
                SendMessage(hTitle, WM_SETFONT, (WPARAM)hFont, TRUE);
            }
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            // Clean up the custom font
            HWND hTitle = GetDlgItem(hDlg, IDC_STATIC_TITLE);
            if (hTitle) {
                HFONT hFont = (HFONT)SendMessage(hTitle, WM_GETFONT, 0, 0);
                if (hFont) {
                    DeleteObject(hFont);
                }
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void MainWindow::showAboutDialog() const
{
    DialogBox(
        Application::getInstance().getHInstance(),
        MAKEINTRESOURCE(IDD_ABOUTBOX),
        m_hwnd,
        AboutDlgProc
    );
}

void MainWindow::setupMenus() const {
    HMENU menuBar = CreateMenu();

    HMENU fileMenu = CreatePopupMenu();
    AppendMenu(fileMenu, MF_STRING, ID_FILE_NEW, L"&New Project\tCtrl+N");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_OPEN, L"&Open Project...\tCtrl+O");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_SAVE, L"&Save Project\tCtrl+S");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_SAVE_AS, L"Save Project &As...\tCtrl+Shift+S");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_CLOSE, L"&Close Project");
    AppendMenu(fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(fileMenu, MF_STRING, ID_FILE_IMPORT_AUDIO, L"&Import Audio...\tCtrl+I");
    AppendMenu(fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(fileMenu, MF_STRING, ID_FILE_EXIT, L"E&xit\tAlt+F4");
    AppendMenu(menuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), L"&File");

    HMENU editMenu = CreatePopupMenu();
    AppendMenu(editMenu, MF_STRING, ID_EDIT_UNDO, L"&Undo\tCtrl+Z");
    AppendMenu(editMenu, MF_STRING, ID_EDIT_REDO, L"&Redo\tCtrl+Y");
    AppendMenu(editMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(editMenu, MF_STRING, ID_EDIT_CUT, L"Cu&t\tCtrl+X");
    AppendMenu(editMenu, MF_STRING, ID_EDIT_COPY, L"&Copy\tCtrl+C");
    AppendMenu(editMenu, MF_STRING, ID_EDIT_PASTE, L"&Paste\tCtrl+V");
    AppendMenu(editMenu, MF_STRING, ID_EDIT_DELETE, L"&Delete\tDel");
    AppendMenu(menuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(editMenu), L"&Edit");

    HMENU trackMenu = CreatePopupMenu();
    AppendMenu(trackMenu, MF_STRING, ID_TRACK_ADD, L"&Add Track\tCtrl+T");
    AppendMenu(trackMenu, MF_STRING, ID_TRACK_DELETE, L"&Delete Track");
    AppendMenu(menuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(trackMenu), L"&Track");

    HMENU transportMenu = CreatePopupMenu();
    AppendMenu(transportMenu, MF_STRING, ID_TRANSPORT_PLAY, L"&Play/Pause\tSpace");
    AppendMenu(transportMenu, MF_STRING, ID_TRANSPORT_STOP, L"&Stop\tEnter");
    AppendMenu(transportMenu, MF_STRING, ID_TRANSPORT_REWIND, L"&Rewind\tHome");
    AppendMenu(transportMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(transportMenu, MF_STRING, ID_TRANSPORT_RECORD, L"&Record\tR");
    AppendMenu(menuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(transportMenu), L"T&ransport");

    HMENU viewMenu = CreatePopupMenu();
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_ZOOM_IN, L"Zoom &In\tCtrl++");
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_ZOOM_OUT, L"Zoom &Out\tCtrl+-");
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_ZOOM_FIT, L"Zoom to &Fit\tCtrl+0");
    AppendMenu(menuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(viewMenu), L"&View");

    HMENU helpMenu = CreatePopupMenu();
    AppendMenu(helpMenu, MF_STRING, ID_HELP_ABOUT, L"&About WavPlayer...");
    AppendMenu(menuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(helpMenu), L"&Help");

    SetMenu(m_hwnd, menuBar);
}

void MainWindow::updateWindowTitle() {
    if (!m_project) {
        return;
    }

    std::wstring title = L"WavPlayer - " + m_project->getProjectName();
    if (m_project->isModified()) {
        title += L" *";
    }

    SetWindowText(m_hwnd, title.c_str());
}

void MainWindow::syncProjectToUI() {
    if (!m_project) {
        return;
    }

    while (!m_timelineView->getTracks().empty()) {
        m_timelineView->removeTrack(0);
    }

    for (const auto& track : m_project->getTracks()) {
        m_timelineView->addTrack(track);
    }

    m_timelineView->setBPM(m_project->getBPM());
    m_transportBar->setBPM(m_project->getBPM());

    resetPlaybackToStart();
    refreshProjectDuration();

    m_timelineView->invalidate();
    resetTrackNumbering();
}

void MainWindow::syncUIToProject() {
    markProjectModified();
}

bool MainWindow::promptSaveIfModified() {
    if (!m_project || !m_project->isModified()) {
        return true;
    }

    const std::wstring message = L"Do you want to save changes to \"" +
        m_project->getProjectName() + L"\"?";

    const int result = MessageBox(m_hwnd, message.c_str(), L"Audio Studio",
        MB_YESNOCANCEL | MB_ICONQUESTION);

    switch (result) {
    case IDYES:
        return saveProject();
    case IDNO:
        return true;
    default:
        return false;
    }
}

void MainWindow::newProject() {
    if (!promptSaveIfModified()) {
        return;
    }

    stop(true);
    m_project->clear();
    addDefaultTrack();
    m_project->setModified(false);

    syncProjectToUI();
    updateWindowTitle();
}

bool MainWindow::openProject() {
    if (!promptSaveIfModified()) {
        return false;
    }

    OPENFILENAME ofn = {};
    wchar_t filename[MAX_PATH] = {};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = Project::FILE_FILTER;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"austd";

    if (!GetOpenFileName(&ofn)) {
        return false;
    }

    stop(true);

    if (!m_project->load(filename)) {
        MessageBox(m_hwnd, L"Failed to open project file", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    syncProjectToUI();
    updateWindowTitle();
    return true;
}

bool MainWindow::saveProject() {
    if (!m_project->hasFilename()) {
        return saveProjectAs();
    }

    if (!m_project->save(m_project->getFilename())) {
        MessageBox(m_hwnd, L"Failed to save project file", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    updateWindowTitle();
    return true;
}

bool MainWindow::saveProjectAs() {
    OPENFILENAME ofn = {};
    wchar_t filename[MAX_PATH] = {};
    wcscpy_s(filename, m_project->getProjectName().c_str());

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = Project::FILE_FILTER;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"austd";

    if (!GetSaveFileName(&ofn)) {
        return false;
    }

    if (!m_project->save(filename)) {
        MessageBox(m_hwnd, L"Failed to save project file", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    updateWindowTitle();
    return true;
}

bool MainWindow::closeProject() {
    if (!promptSaveIfModified()) {
        return false;
    }

    newProject();
    return true;
}

bool MainWindow::importAudioFile() {
    OPENFILENAME ofn = {};
    wchar_t filename[MAX_PATH] = {};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"WAV Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileName(&ofn)) {
        return false;
    }

    return loadAudioFile(filename);
}

bool MainWindow::loadAudioFile(const std::wstring& filename) {
    auto clip = m_project->getOrLoadClip(filename);
    if (!clip) {
        MessageBox(m_hwnd, L"Failed to load audio file", L"Error", MB_OK);
        return false;
    }

    auto& tracks = m_project->getTracks();
    if (!tracks.empty()) {
        TrackRegion region;
        region.clip = clip;
        region.startTime = 0.0;
        region.clipOffset = 0.0;
        region.duration = clip->getDuration();

        tracks[0]->addRegion(region);
        refreshProjectDuration();
        markProjectModified();
    }

    m_timelineView->invalidate();
    return true;
}

void MainWindow::play() {
    ensureAudioEngineTracks();
    refreshProjectDuration();

    if (!m_audioEngine) {
        return;
    }

    // Use the current transport marker (timeline playhead) as the start point
    double startPosition = 0.0;
    if (m_timelineView) {
        startPosition = m_timelineView->getPlayheadPosition();
    }

    if (!m_audioEngine->isPlaying()) {
        m_audioEngine->setPosition(startPosition);
        if (m_transportBar) {
            m_transportBar->setPosition(startPosition);
        }
    }

    if (m_audioEngine->play()) {
        m_transportBar->setPlaying(true);
    }
}

void MainWindow::pause() {
    if (m_audioEngine) {
        m_audioEngine->pause();
    }
    m_transportBar->setPlaying(false);
}

void MainWindow::stop(bool resetPlayhead) {
    double positionBeforeStop = 0.0;

    if (m_timelineView) {
        positionBeforeStop = m_timelineView->getPlayheadPosition();
    }

    if (m_audioEngine) {
        if (m_audioEngine->isRecording()) {
            m_audioEngine->stopRecording();
            if (m_transportBar) {
                m_transportBar->setRecording(false);
            }
        }

        positionBeforeStop = m_audioEngine->getPosition();
        m_audioEngine->stop();
    }

    if (m_transportBar) {
        m_transportBar->setPlaying(false);
    }

    if (resetPlayhead) {
        resetPlaybackToStart();
        return;
    }

    if (m_audioEngine) {
        m_audioEngine->setPosition(positionBeforeStop);
    }
    if (m_timelineView) {
        m_timelineView->setPlayheadPosition(positionBeforeStop);
    }
    if (m_transportBar) {
        m_transportBar->setPosition(positionBeforeStop);
    }
}

void MainWindow::togglePlayPause() {
    if (m_audioEngine->isPlaying()) {
        pause();
    } else {
        play();
    }
}

void MainWindow::startRecording() {
    if (m_audioEngine->isRecording()) {
        return;
    }

    auto armedTrack = m_timelineView->getFirstArmedTrack();
    if (!armedTrack) {
        MessageBox(m_hwnd,
            L"No track is armed for recording.\nClick the 'R' button on a track to arm it.",
            L"Recording", MB_OK | MB_ICONINFORMATION);
        return;
    }

    m_recordingTrack = armedTrack;
    m_recordingStartPosition = m_timelineView->getPlayheadPosition();
    m_recordingTrackIndex = -1;

    const auto& tracks = m_project->getTracks();
    for (size_t i = 0; i < tracks.size(); ++i) {
        if (tracks[i] == armedTrack) {
            m_recordingTrackIndex = static_cast<int>(i) + 1;
            break;
        }
    }

    ensureAudioEngineTracks();

    const double extendedDuration = std::max(
        m_recordingStartPosition + MAX_RECORDING_SECONDS,
        calculateProjectDuration());

    applyProjectDuration(extendedDuration);
    m_audioEngine->setPosition(m_recordingStartPosition);

    if (m_audioEngine->startRecording()) {
        m_transportBar->setRecording(true);
        m_audioEngine->play();
        m_transportBar->setPlaying(true);
        return;
    }

    m_recordingTrack = nullptr;
    m_recordingTrackIndex = -1;
    m_recordingStartPosition = 0.0;
    MessageBox(m_hwnd,
        L"Failed to start recording.\nPlease check your microphone settings.",
        L"Recording Error", MB_OK | MB_ICONERROR);
}

void MainWindow::stopRecording() {
    if (!m_audioEngine->isRecording()) {
        return;
    }

    m_audioEngine->stopRecording();
    m_transportBar->setRecording(false);
}

void MainWindow::toggleRecording() {
    if (m_audioEngine->isRecording()) {
        stopRecording();
    } else {
        startRecording();
    }
}

void MainWindow::onRecordingComplete(std::shared_ptr<AudioClip> clip) {
    if (!clip || clip->getSampleCount() == 0) {
        return;
    }

    autoSaveRecordedClip(std::move(clip));
}

bool MainWindow::autoSaveRecordedClip(std::shared_ptr<AudioClip> clip) {
    std::wstring projectName = m_project->getProjectName();

    int recordingNum = 1;
    if (m_recordingTrackIndex > 0 && m_recordingTrack) {
        recordingNum = static_cast<int>(m_recordingTrack->getRegions().size()) + 1;
    }

    std::wstring directory;
    if (m_project->hasFilename()) {
        std::filesystem::path projectPath(m_project->getFilename());
        directory = projectPath.parent_path().wstring();
    } else {
        wchar_t tempPath[MAX_PATH];
        GetTempPath(MAX_PATH, tempPath);
        directory = tempPath;
    }

    const std::wstring filename = directory + L"\\" + projectName + L"_Track" +
        std::to_wstring(m_recordingTrackIndex) + L"_Take" +
        std::to_wstring(recordingNum) + L".wav";

    if (!clip->saveToFile(filename)) {
        MessageBox(m_hwnd, L"Failed to auto-save recording", L"Error", MB_OK | MB_ICONERROR);
        m_recordingTrack = nullptr;
        m_recordingTrackIndex = -1;
        m_recordingStartPosition = 0.0;
        return false;
    }

    auto targetTrack = m_recordingTrack;
    if (!targetTrack && !m_project->getTracks().empty()) {
        targetTrack = m_project->getTracks()[0];
    }

    if (targetTrack) {
        auto loadedClip = m_project->getOrLoadClip(filename);
        if (loadedClip) {
            TrackRegion region;
            region.clip = loadedClip;
            region.startTime = m_recordingStartPosition;
            region.clipOffset = 0.0;
            region.duration = loadedClip->getDuration();

            targetTrack->addRegion(region);
            refreshProjectDuration();
            markProjectModified();
            m_timelineView->invalidate();
        }
    }

    m_recordingTrack = nullptr;
    m_recordingTrackIndex = -1;
    m_recordingStartPosition = 0.0;
    return true;
}

bool MainWindow::saveRecordedClip(std::shared_ptr<AudioClip> clip) {
    ++m_recordingCount;
    std::wstring projectName = m_project->getProjectName();
    std::wstring defaultName = projectName + L"_Recording_" + std::to_wstring(m_recordingCount);

    OPENFILENAME ofn = {};
    wchar_t filename[MAX_PATH] = {};
    wcscpy_s(filename, defaultName.c_str());

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"WAV Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"wav";
    ofn.lpstrTitle = L"Save Recording";

    if (!GetSaveFileName(&ofn)) {
        m_recordingTrack = nullptr;
        m_recordingStartPosition = 0.0;
        return false;
    }

    if (!clip->saveToFile(filename)) {
        MessageBox(m_hwnd, L"Failed to save recording", L"Error", MB_OK | MB_ICONERROR);
        m_recordingTrack = nullptr;
        m_recordingStartPosition = 0.0;
        return false;
    }

    auto targetTrack = m_recordingTrack;
    if (!targetTrack && !m_project->getTracks().empty()) {
        targetTrack = m_project->getTracks()[0];
    }

    if (targetTrack) {
        auto loadedClip = m_project->getOrLoadClip(filename);
        if (loadedClip) {
            TrackRegion region;
            region.clip = loadedClip;
            region.startTime = m_recordingStartPosition;
            region.clipOffset = 0.0;
            region.duration = loadedClip->getDuration();

            targetTrack->addRegion(region);
            refreshProjectDuration();
            markProjectModified();
            m_timelineView->invalidate();
        }
    }

    m_recordingTrack = nullptr;
    m_recordingStartPosition = 0.0;
    return true;
}

void MainWindow::updatePlaybackPosition() {
    if (m_audioEngine->isPlaying()) {
        const double pos = m_audioEngine->getPosition();
        m_transportBar->setPosition(pos);
        m_timelineView->setPlayheadPosition(pos);
    }

    if (m_audioEngine->isRecording()) {
        const double recordDuration = m_audioEngine->getRecordingDuration();
        m_transportBar->setPosition(recordDuration);
        m_timelineView->setPlayheadPosition(recordDuration);
    }
}

void MainWindow::onResize(int width, int height) {
    const int timelineHeight = height - TRANSPORT_HEIGHT;

    if (m_timelineView) {
        SetWindowPos(m_timelineView->getHWND(), nullptr,
            0, 0, width, timelineHeight,
            SWP_NOZORDER);
    }

    if (m_transportBar) {
        SetWindowPos(m_transportBar->getHWND(), nullptr,
            0, timelineHeight, width, TRANSPORT_HEIGHT,
            SWP_NOZORDER);
    }
}

void MainWindow::onCommand(int id) {
    switch (id) {
    case ID_FILE_NEW:
        newProject();
        break;
    case ID_FILE_OPEN:
        openProject();
        break;
    case ID_FILE_SAVE:
        saveProject();
        break;
    case ID_FILE_SAVE_AS:
        saveProjectAs();
        break;
    case ID_FILE_CLOSE:
        closeProject();
        break;
    case ID_FILE_IMPORT_AUDIO:
        importAudioFile();
        break;
    case ID_FILE_EXIT:
        onClose();
        break;
    case ID_TRANSPORT_PLAY:
        togglePlayPause();
        break;
    case ID_TRANSPORT_STOP:
        stop();
        break;
    case ID_TRANSPORT_REWIND:
        resetPlaybackToStart();
        break;
    case ID_TRANSPORT_RECORD:
        toggleRecording();
        break;
    case ID_TRACK_ADD:
        handleTrackAdd();
        break;
    case ID_TRACK_DELETE:
        handleTrackDelete();
        break;
    case ID_VIEW_ZOOM_IN:
        m_timelineView->setPixelsPerSecond(m_timelineView->getPixelsPerSecond() * 1.5);
        break;
    case ID_VIEW_ZOOM_OUT:
        m_timelineView->setPixelsPerSecond(m_timelineView->getPixelsPerSecond() / 1.5);
        break;
    case ID_HELP_ABOUT:
        showAboutDialog();
        break;

    default:
        break;
    }
}

void MainWindow::onDropFiles(HDROP hDrop) {
    wchar_t filename[MAX_PATH];
    UINT count = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);

    for (UINT i = 0; i < count; ++i) {
        DragQueryFile(hDrop, i, filename, MAX_PATH);

        if (PathMatchSpec(filename, L"*.wav")) {
            loadAudioFile(filename);
        } else if (PathMatchSpec(filename, L"*.austd")) {
            if (promptSaveIfModified()) {
                stop(true);
                if (m_project->load(filename)) {
                    syncProjectToUI();
                    updateWindowTitle();
                }
            }
        }
    }

    DragFinish(hDrop);
}

void MainWindow::onClose() {
    if (promptSaveIfModified()) {
        DestroyWindow(m_hwnd);
    }
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* window = nullptr;

    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->m_hwnd = hwnd;
    } else {
        window = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (window) {
        switch (msg) {
        case WM_SIZE:
            window->onResize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_COMMAND:
            window->onCommand(LOWORD(wParam));
            return 0;
        case WM_DROPFILES:
            window->onDropFiles(reinterpret_cast<HDROP>(wParam));
            return 0;
        case WM_TIMER:
            if (wParam == TIMER_PLAYBACK) {
                window->updatePlaybackPosition();
            }
            return 0;
        case WM_KEYDOWN:
            switch (wParam) {
            case VK_SPACE:
                window->togglePlayPause();
                return 0;
            case VK_RETURN:
                window->stop();
                return 0;
            case VK_HOME:
                window->resetPlaybackToStart();
                return 0;
            case 'N':
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    window->newProject();
                    return 0;
                }
                break;
            case 'O':
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    window->openProject();
                    return 0;
                }
                break;
            case 'S':
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    if (GetKeyState(VK_SHIFT) & 0x8000) {
                        window->saveProjectAs();
                    } else {
                        window->saveProject();
                    }
                    return 0;
                }
                break;
            case 'I':
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    window->importAudioFile();
                    return 0;
                }
                break;
            case 'T':
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    window->handleTrackAdd();
                    return 0;
                }
                break;
            case 'R':
                window->toggleRecording();
                return 0;
            default:
                break;
            }
            break;
        case WM_CLOSE:
            window->onClose();
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}