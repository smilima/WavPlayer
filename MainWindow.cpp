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
    ID_FILE_OPEN = 1001,
    ID_FILE_SAVE,
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
    m_timelineView->setBPM(m_bpm);
    
    // Set playhead callback
    m_timelineView->setPlayheadCallback([this](double time) {
        m_audioEngine->setPosition(time);
        m_transportBar->setPosition(time);
    });

    // Transport bar
    m_transportBar = std::make_unique<TransportBar>();
    m_transportBar->create(m_hwnd, 0, timelineHeight, clientWidth, transportHeight);
    m_transportBar->setBPM(m_bpm);
    
    // Setup transport callbacks
    m_transportBar->setPlayCallback([this]() { play(); });
    m_transportBar->setPauseCallback([this]() { pause(); });
    m_transportBar->setStopCallback([this]() { stop(); });
    m_transportBar->setRewindCallback([this]() {
        m_audioEngine->setPosition(0);
        m_timelineView->setPlayheadPosition(0);
        m_transportBar->setPosition(0);
    });

    // Add a default track
    auto defaultTrack = std::make_shared<Track>(L"Track 1");
    defaultTrack->setColor(0xFF4A90D9);
    m_timelineView->addTrack(defaultTrack);

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    // Start playback position timer (30 fps update)
    m_timerId = SetTimer(m_hwnd, TIMER_PLAYBACK, 33, nullptr);

    return true;
}

void MainWindow::setupMenus() {
    HMENU menuBar = CreateMenu();
    
    // File menu
    HMENU fileMenu = CreatePopupMenu();
    AppendMenu(fileMenu, MF_STRING, ID_FILE_OPEN, L"&Open Audio...\tCtrl+O");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_SAVE, L"&Save Project\tCtrl+S");
    AppendMenu(fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(fileMenu, MF_STRING, ID_FILE_EXIT, L"E&xit");
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
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)transportMenu, L"T&ransport");
    
    // View menu
    HMENU viewMenu = CreatePopupMenu();
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_ZOOM_IN, L"Zoom &In\tCtrl++");
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_ZOOM_OUT, L"Zoom &Out\tCtrl+-");
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_ZOOM_FIT, L"Zoom to &Fit\tCtrl+0");
    AppendMenu(menuBar, MF_POPUP, (UINT_PTR)viewMenu, L"&View");
    
    SetMenu(m_hwnd, menuBar);
}

bool MainWindow::loadAudioFile(const std::wstring& filename) {
    auto clip = std::make_shared<AudioClip>();
    if (!clip->loadFromFile(filename)) {
        MessageBox(m_hwnd, L"Failed to load audio file", L"Error", MB_OK);
        return false;
    }
    
    m_loadedClips.push_back(clip);
    
    // Add to first track as a region
    auto& tracks = m_timelineView->getTracks();
    if (!tracks.empty()) {
        TrackRegion region;
        region.clip = clip;
        region.startTime = 0.0;
        region.clipOffset = 0.0;
        region.duration = clip->getDuration();
        
        tracks[0]->addRegion(region);
        
        // Set clip for playback
        m_audioEngine->setClip(clip);
        
        // Update transport with duration
        m_transportBar->setDuration(clip->getDuration());
    }
    
    m_timelineView->invalidate();
    return true;
}

void MainWindow::play() {
    m_audioEngine->play();
    m_transportBar->setPlaying(true);
    m_timelineView->setFollowPlayhead(true);  // Re-enable follow mode on play
}

void MainWindow::pause() {
    m_audioEngine->pause();
    m_transportBar->setPlaying(false);
}

void MainWindow::stop() {
    m_audioEngine->stop();
    m_transportBar->setPlaying(false);
    m_transportBar->setPosition(0);
    m_timelineView->setPlayheadPosition(0);
    m_timelineView->setFollowPlayhead(true);  // Re-enable follow mode on stop
}

void MainWindow::togglePlayPause() {
    if (m_audioEngine->isPlaying()) {
        pause();
    } else {
        play();
    }
}

void MainWindow::updatePlaybackPosition() {
    if (m_audioEngine->isPlaying()) {
        double pos = m_audioEngine->getPosition();
        m_transportBar->setPosition(pos);
        m_timelineView->setPlayheadPosition(pos);
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
    case ID_FILE_OPEN:
        {
            OPENFILENAME ofn = {};
            wchar_t filename[MAX_PATH] = {};
            
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = m_hwnd;
            ofn.lpstrFilter = L"WAV Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = filename;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            
            if (GetOpenFileName(&ofn)) {
                loadAudioFile(filename);
            }
        }
        break;
        
    case ID_FILE_EXIT:
        PostQuitMessage(0);
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
        
    case ID_TRACK_ADD:
        {
            static int trackNum = 2;
            auto track = std::make_shared<Track>(L"Track " + std::to_wstring(trackNum++));
            // Cycle through colors
            uint32_t colors[] = {0xFF4A90D9, 0xFF5CB85C, 0xFFD9534F, 0xFFF0AD4E, 0xFF9B59B6};
            track->setColor(colors[(trackNum - 2) % 5]);
            m_timelineView->addTrack(track);
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
        
        // Check if it's a WAV file
        if (PathMatchSpec(filename, L"*.wav")) {
            loadAudioFile(filename);
        }
    }
    
    DragFinish(hDrop);
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
            }
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
