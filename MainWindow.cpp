#include "MainWindow.h"
#include "Application.h"
#include <Shlwapi.h>
#include <commdlg.h>
#include <shellapi.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Shell32.lib")

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
};

// Timer ID for playback position updates
constexpr UINT_PTR TIMER_PLAYBACK = 1;

MainWindow::MainWindow() {}

MainWindow::~MainWindow() {
    if (m_timerId) {
        KillTimer(m_hwnd, m_timerId);
    }
}

bool MainWindow::create(const wchar_t* title, int width, int height) {
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = Application::getInstance().getHInstance();
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"DAWMainWindow";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    
    RegisterClassEx(&wc);

    // Create window
    m_hwnd = CreateWindowEx(
        WS_EX_ACCEPTFILES,  // Accept drag-drop files
        L"DAWMainWindow",
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        Application::getInstance().getHInstance(),
        this
    );

    if (!m_hwnd) return false;

    // Setup menus
    setupMenus();

    // Create audio engine
    m_audioEngine = std::make_unique<AudioEngine>();
    if (!m_audioEngine->initialize(44100, 2)) {
        MessageBox(m_hwnd, L"Failed to initialize audio engine", L"Error", MB_OK);
    }

    // Create project
    m_project = std::make_unique<Project>();

    // Create child views
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int clientWidth = rc.right - rc.left;
    int clientHeight = rc.bottom - rc.top;

    int transportHeight = 50;
    int timelineHeight = clientHeight - transportHeight;

    // Timeline view
    m_timelineView = std::make_unique<TimelineView>();
    m_timelineView->create(m_hwnd, 0, 0, clientWidth, timelineHeight);
    
    // Set playhead callback
    m_timelineView->setPlayheadCallback([this](double time) {
        m_audioEngine->setPosition(time);
        m_transportBar->setPosition(time);
    });

    // Transport bar
    m_transportBar = std::make_unique<TransportBar>();
    m_transportBar->create(m_hwnd, 0, timelineHeight, clientWidth, transportHeight);
    
    // Setup transport callbacks
    m_transportBar->setPlayCallback([this]() { play(); });
    m_transportBar->setPauseCallback([this]() { pause(); });
    m_transportBar->setStopCallback([this]() { stop(); });
    m_transportBar->setRewindCallback([this]() {
        m_audioEngine->setPosition(0);
        m_timelineView->setPlayheadPosition(0);
        m_transportBar->setPosition(0);
    });
    m_transportBar->setRecordCallback([this]() { toggleRecording(); });
    
    // Set recording complete callback
    m_audioEngine->setRecordingCallback([this](std::shared_ptr<AudioClip> clip) {
        onRecordingComplete(clip);
    });

    // Sync project to UI
    syncProjectToUI();

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    // Start playback position timer (30 fps update)
    m_timerId = SetTimer(m_hwnd, TIMER_PLAYBACK, 33, nullptr);

    updateWindowTitle();
    return true;
}

void MainWindow::setupMenus() {
    HMENU menuBar = CreateMenu();
    
    // File menu
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
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)fileMenu, L"&File");
    
    // Edit menu
    HMENU editMenu = CreatePopupMenu();
    AppendMenu(editMenu, MF_STRING, ID_EDIT_UNDO, L"&Undo\tCtrl+Z");
    AppendMenu(editMenu, MF_STRING, ID_EDIT_REDO, L"&Redo\tCtrl+Y");
    AppendMenu(editMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(editMenu, MF_STRING, ID_EDIT_CUT, L"Cu&t\tCtrl+X");
    AppendMenu(editMenu, MF_STRING, ID_EDIT_COPY, L"&Copy\tCtrl+C");
    AppendMenu(editMenu, MF_STRING, ID_EDIT_PASTE, L"&Paste\tCtrl+V");
    AppendMenu(editMenu, MF_STRING, ID_EDIT_DELETE, L"&Delete\tDel");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)editMenu, L"&Edit");
    
    // Track menu
    HMENU trackMenu = CreatePopupMenu();
    AppendMenu(trackMenu, MF_STRING, ID_TRACK_ADD, L"&Add Track\tCtrl+T");
    AppendMenu(trackMenu, MF_STRING, ID_TRACK_DELETE, L"&Delete Track");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)trackMenu, L"&Track");
    
    // Transport menu
    HMENU transportMenu = CreatePopupMenu();
    AppendMenu(transportMenu, MF_STRING, ID_TRANSPORT_PLAY, L"&Play/Pause\tSpace");
    AppendMenu(transportMenu, MF_STRING, ID_TRANSPORT_STOP, L"&Stop\tEnter");
    AppendMenu(transportMenu, MF_STRING, ID_TRANSPORT_REWIND, L"&Rewind\tHome");
    AppendMenu(transportMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(transportMenu, MF_STRING, ID_TRANSPORT_RECORD, L"&Record\tR");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)transportMenu, L"T&ransport");
    
    // View menu
    HMENU viewMenu = CreatePopupMenu();
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_ZOOM_IN, L"Zoom &In\tCtrl++");
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_ZOOM_OUT, L"Zoom &Out\tCtrl+-");
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_ZOOM_FIT, L"Zoom to &Fit\tCtrl+0");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)viewMenu, L"&View");
    
    SetMenu(m_hwnd, menuBar);
}

void MainWindow::updateWindowTitle() {
    std::wstring title = L"Audio Studio - ";
    title += m_project->getProjectName();
    
    if (m_project->isModified()) {
        title += L" *";
    }
    
    SetWindowText(m_hwnd, title.c_str());
}

void MainWindow::syncProjectToUI() {
    // Clear timeline tracks
    while (!m_timelineView->getTracks().empty()) {
        m_timelineView->removeTrack(0);
    }
    
    // Add tracks from project
    for (auto& track : m_project->getTracks()) {
        m_timelineView->addTrack(track);
    }
    
    // Update BPM
    m_timelineView->setBPM(m_project->getBPM());
    m_transportBar->setBPM(m_project->getBPM());
    
    // Reset playhead
    m_timelineView->setPlayheadPosition(0);
    m_transportBar->setPosition(0);
    m_transportBar->setDuration(0);
    
    // Find longest region for duration
    double maxDuration = 0;
    for (const auto& track : m_project->getTracks()) {
        for (const auto& region : track->getRegions()) {
            maxDuration = std::max(maxDuration, region.startTime + region.duration);
        }
    }
    m_transportBar->setDuration(maxDuration);
    
    // Set up audio engine for track mixing
    m_audioEngine->setTracks(&m_project->getTracks());
    m_audioEngine->setDuration(maxDuration);
    
    m_timelineView->invalidate();
}

void MainWindow::syncUIToProject() {
    // The tracks are shared, so they're already synced
    // Just mark as modified
    m_project->setModified(true);
    updateWindowTitle();
}

bool MainWindow::promptSaveIfModified() {
    if (!m_project->isModified()) {
        return true;
    }
    
    std::wstring message = L"Do you want to save changes to \"" + 
                           m_project->getProjectName() + L"\"?";
    
    int result = MessageBox(m_hwnd, message.c_str(), L"Audio Studio", 
                            MB_YESNOCANCEL | MB_ICONQUESTION);
    
    switch (result) {
        case IDYES:
            return saveProject();
        case IDNO:
            return true;
        case IDCANCEL:
        default:
            return false;
    }
}

void MainWindow::newProject() {
    if (!promptSaveIfModified()) {
        return;
    }
    
    stop();
    
    m_project->clear();
    
    // Add a default track
    auto track = std::make_shared<Track>(L"Track 1");
    track->setColor(0xFF4A90D9);
    m_project->addTrack(track);
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
    
    stop();
    
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
    
    // Pre-fill with current project name
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
    // Load clip through project (uses cache)
    auto clip = m_project->getOrLoadClip(filename);
    if (!clip) {
        MessageBox(m_hwnd, L"Failed to load audio file", L"Error", MB_OK);
        return false;
    }
    
    // Add to first track as a region
    auto& tracks = m_project->getTracks();
    if (!tracks.empty()) {
        TrackRegion region;
        region.clip = clip;
        region.startTime = 0.0;
        region.clipOffset = 0.0;
        region.duration = clip->getDuration();
        
        tracks[0]->addRegion(region);
        
        // Update duration (find max from all tracks)
        double maxDuration = 0;
        for (const auto& track : tracks) {
            for (const auto& r : track->getRegions()) {
                maxDuration = std::max(maxDuration, r.endTime());
            }
        }
        m_transportBar->setDuration(maxDuration);
        m_audioEngine->setDuration(maxDuration);
        
        m_project->setModified(true);
        updateWindowTitle();
    }
    
    m_timelineView->invalidate();
    return true;
}

void MainWindow::play() {
    // Ensure tracks pointer is set
    m_audioEngine->setTracks(&m_project->getTracks());
    
    // Calculate total duration from all tracks and regions
    double maxDuration = 0;
    int regionCount = 0;
    for (const auto& track : m_project->getTracks()) {
        for (const auto& region : track->getRegions()) {
            maxDuration = std::max(maxDuration, region.endTime());
            regionCount++;
        }
    }
    
    // Debug: show what we found
    wchar_t debug[256];
    swprintf_s(debug, L"Tracks: %zu, Regions: %d, Duration: %.2f", 
               m_project->getTracks().size(), regionCount, maxDuration);
    MessageBox(m_hwnd, debug, L"Play Debug", MB_OK);
    
    m_audioEngine->setDuration(maxDuration);
    m_transportBar->setDuration(maxDuration);
    
    if (m_audioEngine->play()) {
        m_transportBar->setPlaying(true);
        m_timelineView->setFollowPlayhead(true);
    } else {
        MessageBox(m_hwnd, L"AudioEngine::play() returned false", L"Play Failed", MB_OK);
    }
}

void MainWindow::pause() {
    m_audioEngine->pause();
    m_transportBar->setPlaying(false);
}

void MainWindow::stop() {
    // Stop recording if in progress
    if (m_audioEngine->isRecording()) {
        stopRecording();
    }
    
    m_audioEngine->stop();
    m_transportBar->setPlaying(false);
    m_transportBar->setPosition(0);
    m_timelineView->setPlayheadPosition(0);
    m_timelineView->setFollowPlayhead(true);
}

void MainWindow::togglePlayPause() {
    if (m_audioEngine->isPlaying()) {
        pause();
    } else {
        play();
    }
}

void MainWindow::startRecording() {
    if (m_audioEngine->isRecording()) return;
    
    // Check if any track is armed
    auto armedTrack = m_timelineView->getFirstArmedTrack();
    if (!armedTrack) {
        MessageBox(m_hwnd, L"No track is armed for recording.\nClick the 'R' button on a track to arm it.",
                   L"Recording", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Remember which track we're recording to and where
    m_recordingTrack = armedTrack;
    m_recordingStartPosition = m_timelineView->getPlayheadPosition();
    
    if (m_audioEngine->startRecording()) {
        m_transportBar->setRecording(true);
        m_timelineView->setFollowPlayhead(true);
        
        // Also start playback so we can hear what we're recording over
        // (optional - can be controlled by user preference)
    }
    else {
        m_recordingTrack = nullptr;
        m_recordingStartPosition = 0.0;
        MessageBox(m_hwnd, L"Failed to start recording.\nPlease check your microphone settings.",
                   L"Recording Error", MB_OK | MB_ICONERROR);
    }
}

void MainWindow::stopRecording() {
    if (!m_audioEngine->isRecording()) return;
    
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
    
    // Ask user where to save the recording
    if (saveRecordedClip(clip)) {
        // Clip was saved and will be added to project via loadAudioFile
    }
}

bool MainWindow::saveRecordedClip(std::shared_ptr<AudioClip> clip) {
    // Generate default filename
    ++m_recordingCount;
    std::wstring defaultName = L"Recording_" + std::to_wstring(m_recordingCount);
    
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
        return false;
    }
    
    // Save the clip
    if (!clip->saveToFile(filename)) {
        MessageBox(m_hwnd, L"Failed to save recording", L"Error", MB_OK | MB_ICONERROR);
        m_recordingTrack = nullptr;
        return false;
    }
    
    // Add clip to the recording track (or first track if none specified)
    auto targetTrack = m_recordingTrack;
    if (!targetTrack && !m_project->getTracks().empty()) {
        targetTrack = m_project->getTracks()[0];
    }
    
    if (targetTrack) {
        // Load clip into project cache
        auto loadedClip = m_project->getOrLoadClip(filename);
        if (loadedClip) {
            TrackRegion region;
            region.clip = loadedClip;
            region.startTime = m_recordingStartPosition;  // Start where recording began
            region.clipOffset = 0.0;
            region.duration = loadedClip->getDuration();
            
            targetTrack->addRegion(region);
            
            // Update duration display
            double maxDuration = 0.0;
            for (const auto& t : m_project->getTracks()) {
                for (const auto& r : t->getRegions()) {
                    maxDuration = std::max(maxDuration, r.endTime());
                }
            }
            m_transportBar->setDuration(maxDuration);
            m_audioEngine->setDuration(maxDuration);
            
            m_project->setModified(true);
            updateWindowTitle();
            m_timelineView->invalidate();
        }
    }
    
    m_recordingTrack = nullptr;
    m_recordingStartPosition = 0.0;
    return true;
}

void MainWindow::updatePlaybackPosition() {
    if (m_audioEngine->isPlaying()) {
        double pos = m_audioEngine->getPosition();
        m_transportBar->setPosition(pos);
        m_timelineView->setPlayheadPosition(pos);
    }
    
    // Update recording duration
    if (m_audioEngine->isRecording()) {
        double recordDuration = m_audioEngine->getRecordingDuration();
        // Could display this somewhere - for now the transport shows it via position
        m_transportBar->setPosition(recordDuration);
        m_timelineView->setPlayheadPosition(recordDuration);
    }
}

void MainWindow::onResize(int width, int height) {
    int transportHeight = 50;
    int timelineHeight = height - transportHeight;
    
    if (m_timelineView) {
        SetWindowPos(m_timelineView->getHWND(), nullptr, 
                     0, 0, width, timelineHeight, 
                     SWP_NOZORDER);
    }
    
    if (m_transportBar) {
        SetWindowPos(m_transportBar->getHWND(), nullptr,
                     0, timelineHeight, width, transportHeight,
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
        m_audioEngine->setPosition(0);
        m_timelineView->setPlayheadPosition(0);
        m_transportBar->setPosition(0);
        break;
        
    case ID_TRANSPORT_RECORD:
        toggleRecording();
        break;
        
    case ID_TRACK_ADD:
        {
            static int trackNum = 2;
            auto track = std::make_shared<Track>(L"Track " + std::to_wstring(trackNum++));
            // Cycle through colors
            uint32_t colors[] = {0xFF4A90D9, 0xFF5CB85C, 0xFFD9534F, 0xFFF0AD4E, 0xFF9B59B6};
            track->setColor(colors[(trackNum - 2) % 5]);
            track->setVisible(true);  // Make manually added tracks visible
            m_project->addTrack(track);
            m_timelineView->addTrack(track);
            updateWindowTitle();
        }
        break;
        
    case ID_VIEW_ZOOM_IN:
        m_timelineView->setPixelsPerSecond(m_timelineView->getPixelsPerSecond() * 1.5);
        break;
        
    case ID_VIEW_ZOOM_OUT:
        m_timelineView->setPixelsPerSecond(m_timelineView->getPixelsPerSecond() / 1.5);
        break;
    }
}

void MainWindow::onDropFiles(HDROP hDrop) {
    wchar_t filename[MAX_PATH];
    UINT count = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
    
    for (UINT i = 0; i < count; ++i) {
        DragQueryFile(hDrop, i, filename, MAX_PATH);
        
        // Check file type
        if (PathMatchSpec(filename, L"*.wav")) {
            loadAudioFile(filename);
        }
        else if (PathMatchSpec(filename, L"*.austd")) {
            if (promptSaveIfModified()) {
                stop();
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
    }
    else {
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
                window->m_audioEngine->setPosition(0);
                window->m_timelineView->setPlayheadPosition(0);
                window->m_transportBar->setPosition(0);
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
                    window->onCommand(ID_TRACK_ADD);
                    return 0;
                }
                break;
            case 'R':
                // R key for recording (no modifier needed)
                window->toggleRecording();
                return 0;
            }
            break;
            
        case WM_CLOSE:
            window->onClose();
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
