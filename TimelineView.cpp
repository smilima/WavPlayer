#include "TimelineView.h"
#include "Application.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <CommCtrl.h>

#pragma comment(lib, "Comctl32.lib")

TimelineView::~TimelineView() {
    if (m_editControl) {
        DestroyWindow(m_editControl);
        m_editControl = nullptr;
    }
    if (m_editFont) {
        DeleteObject(m_editFont);
        m_editFont = nullptr;
    }
}

void TimelineView::addTrack(std::shared_ptr<Track> track) {
    m_tracks.push_back(track);
    invalidate();
}

void TimelineView::removeTrack(size_t index) {
    if (index < m_tracks.size()) {
        m_tracks.erase(m_tracks.begin() + index);
        invalidate();
    }
}

bool TimelineView::hasArmedTrack() const {
    for (const auto& track : m_tracks) {
        if (track->isArmed() && track->isVisible()) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<Track> TimelineView::getFirstArmedTrack() const {
    for (const auto& track : m_tracks) {
        if (track->isArmed() && track->isVisible()) {
            return track;
        }
    }
    return nullptr;
}

void TimelineView::setPlayheadPosition(double seconds) {
    m_playheadPosition = std::max(0.0, seconds);
    
    // Auto-scroll to keep playhead visible if follow mode is enabled
    if (m_followPlayhead) {
        ensurePlayheadVisible();
    }
    
    invalidate();
}

void TimelineView::ensurePlayheadVisible() {
    int playheadPixel = timeToPixel(m_playheadPosition);
    int contentWidth = getWidth() - TRACK_HEADER_WIDTH;
    
    // Define margins - when playhead reaches right margin, scroll
    int rightMargin = TRACK_HEADER_WIDTH + static_cast<int>(contentWidth * 0.85);  // 85% from left
    int leftMargin = TRACK_HEADER_WIDTH + static_cast<int>(contentWidth * 0.15);   // 15% from left
    
    if (playheadPixel > rightMargin) {
        // Playhead went past right edge - scroll so playhead is at 25% from left
        double targetScrollX = m_playheadPosition - (contentWidth * 0.25) / m_pixelsPerSecond;
        m_scrollX = std::max(0.0, targetScrollX);
    }
    else if (playheadPixel < leftMargin && m_scrollX > 0) {
        // Playhead went past left edge (e.g., when rewinding) - scroll to show it
        double targetScrollX = m_playheadPosition - (contentWidth * 0.25) / m_pixelsPerSecond;
        m_scrollX = std::max(0.0, targetScrollX);
    }
}

void TimelineView::setPixelsPerSecond(double pps) {
    m_pixelsPerSecond = std::clamp(pps, 10.0, 1000.0);
    invalidate();
}

void TimelineView::setScrollX(double x) {
    m_scrollX = std::max(0.0, x);
    invalidate();
}

void TimelineView::setScrollY(int y) {
    m_scrollY = std::max(0, y);
    invalidate();
}

void TimelineView::onRender(ID2D1RenderTarget* rt) {
    // Clear background
    fillRect(0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight()), 
             DAWColors::Background);
    
    // Draw grid first (under everything)
    if (m_showGrid) {
        drawGrid(rt);
    }
    
    // Draw tracks
    drawTracks(rt);
    
    // Draw ruler at top
    drawRuler(rt);
    
    // Draw playhead on top
    drawPlayhead(rt);
    
    // Track header backgrounds (drawn last to cover grid)
    fillRect(0, 0, TRACK_HEADER_WIDTH, static_cast<float>(getHeight()), 
             DAWColors::TrackHeader);
    
    // Redraw track headers on top (only for visible tracks)
    float y = (float)(RULER_HEIGHT - m_scrollY);
    for (auto& track : m_tracks) {
        // Skip tracks that aren't visible
        if (!track->isVisible()) continue;
        
        if (y + track->getHeight() > RULER_HEIGHT && y < getHeight()) {
            drawTrackHeader(rt, *track, y, static_cast<float>(track->getHeight()));
        }
        y += track->getHeight();
    }
    
    // Corner box (top-left)
    fillRect(0, 0, TRACK_HEADER_WIDTH, RULER_HEIGHT, DAWColors::TrackHeader);
    drawRect(0, 0, TRACK_HEADER_WIDTH, RULER_HEIGHT, DAWColors::GridLine);
}

void TimelineView::drawRuler(ID2D1RenderTarget* rt) {
    float rulerY = 0;
    float contentX = static_cast<float>(TRACK_HEADER_WIDTH);
    float contentWidth = static_cast<float>(getWidth() - TRACK_HEADER_WIDTH);
    
    // Ruler background
    fillRect(contentX, rulerY, contentWidth, RULER_HEIGHT, DAWColors::Timeline);
    
    // Calculate time interval for markers based on zoom level
    double secondsPerPixel = 1.0 / m_pixelsPerSecond;
    double visibleSeconds = contentWidth * secondsPerPixel;
    
    // Choose appropriate interval
    double interval = 1.0;  // 1 second default
    if (visibleSeconds > 60) interval = 10.0;
    else if (visibleSeconds > 30) interval = 5.0;
    else if (visibleSeconds > 10) interval = 2.0;
    else if (visibleSeconds < 2) interval = 0.5;
    else if (visibleSeconds < 1) interval = 0.1;
    
    double startTime = m_scrollX - fmod(m_scrollX, interval);
    double endTime = m_scrollX + visibleSeconds + interval;
    
    for (double t = startTime; t <= endTime; t += interval) {
        int x = timeToPixel(t);
        if (x < TRACK_HEADER_WIDTH || x > getWidth()) continue;
        
        // Marker line
        bool isMajor = fmod(t, interval * 4) < 0.001;
        float lineHeight = isMajor ? RULER_HEIGHT * 0.6f : RULER_HEIGHT * 0.3f;
        
        drawLine(static_cast<float>(x), RULER_HEIGHT - lineHeight,
                 static_cast<float>(x), static_cast<float>(RULER_HEIGHT),
                 isMajor ? DAWColors::GridLineMajor : DAWColors::GridLine, 1.0f);
        
        // Time label for major markers
        if (isMajor || interval >= 1.0) {
            int mins = static_cast<int>(t) / 60;
            int secs = static_cast<int>(t) % 60;
            std::wostringstream ss;
            ss << mins << L":" << std::setfill(L'0') << std::setw(2) << secs;
            drawText(ss.str(), static_cast<float>(x + 4), 4, 
                     DAWColors::TimelineText, 60, 20);
        }
    }
    
    // Bottom border
    drawLine(contentX, RULER_HEIGHT, static_cast<float>(getWidth()), RULER_HEIGHT,
             DAWColors::GridLine, 1.0f);
}

void TimelineView::drawGrid(ID2D1RenderTarget* rt) {
    float contentX = static_cast<float>(TRACK_HEADER_WIDTH);
    float contentWidth = static_cast<float>(getWidth() - TRACK_HEADER_WIDTH);
    
    // Calculate beat interval
    double secondsPerBeat = 60.0 / m_bpm;
    double beatsPerBar = 4.0;
    double secondsPerBar = secondsPerBeat * beatsPerBar;
    
    // Choose grid resolution based on zoom
    double secondsPerPixel = 1.0 / m_pixelsPerSecond;
    double visibleSeconds = contentWidth * secondsPerPixel;
    
    double gridInterval = secondsPerBeat;
    if (visibleSeconds > 30) gridInterval = secondsPerBar;
    else if (visibleSeconds < 5) gridInterval = secondsPerBeat / 2;
    else if (visibleSeconds < 2) gridInterval = secondsPerBeat / 4;
    
    double startTime = m_scrollX - fmod(m_scrollX, gridInterval);
    double endTime = m_scrollX + visibleSeconds + gridInterval;
    
    for (double t = startTime; t <= endTime; t += gridInterval) {
        int x = timeToPixel(t);
        if (x < TRACK_HEADER_WIDTH || x > getWidth()) continue;
        
        // Check if this is a bar line
        bool isBarLine = fmod(t, secondsPerBar) < 0.001;
        
        drawLine(static_cast<float>(x), static_cast<float>(RULER_HEIGHT),
                 static_cast<float>(x), static_cast<float>(getHeight()),
                 isBarLine ? DAWColors::GridLineMajor : DAWColors::GridLine, 
                 isBarLine ? 1.0f : 0.5f);
    }
}

void TimelineView::drawTracks(ID2D1RenderTarget* rt) {
    float y = static_cast<float>(RULER_HEIGHT - m_scrollY);
    int visibleTrackIndex = 0;  // Count only visible tracks for alternating colors
    
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        auto& track = m_tracks[i];
        
        // Skip tracks that aren't visible
        if (!track->isVisible()) continue;
        
        float height = static_cast<float>(track->getHeight());
        
        if (y + height > RULER_HEIGHT && y < getHeight()) {
            // Track content area background
            float contentX = static_cast<float>(TRACK_HEADER_WIDTH);
            float contentWidth = static_cast<float>(getWidth() - TRACK_HEADER_WIDTH);
            
            // Alternate track colors based on visible track index
            Color bgColor = (visibleTrackIndex % 2 == 0) ? DAWColors::TrackBackground : 
                            Color(DAWColors::TrackBackground.r * 1.1f,
                                  DAWColors::TrackBackground.g * 1.1f,
                                  DAWColors::TrackBackground.b * 1.1f);
            
            fillRect(contentX, y, contentWidth, height, bgColor);
            
            // Draw track content (regions/waveforms)
            drawTrackContent(rt, *track, y, height);
            
            // Track separator line
            drawLine(0, y + height, static_cast<float>(getWidth()), y + height,
                     DAWColors::GridLine, 1.0f);
        }
        
        y += height;
        ++visibleTrackIndex;
    }
}

void TimelineView::drawTrackHeader(ID2D1RenderTarget* rt, Track& track, 
                                    float y, float height) {
    // Header background
    fillRect(0, y, TRACK_HEADER_WIDTH, height, DAWColors::TrackHeader);
    
    // Track color indicator
    fillRect(0, y, 4, height, Color(track.getColor()));
    
    // Track name
    drawText(track.getName(), 12, y + 8, DAWColors::TextPrimary, 
             TRACK_HEADER_WIDTH - 20, 20);
    
    // Mute/Solo/Arm buttons
    float btnY = y + height - 28;
    float btnSize = 20;
    float btnSpacing = 4;
    float btnX = 12;
    
    // Mute button
    Color muteColor = track.isMuted() ? Color(0.9f, 0.3f, 0.3f) : DAWColors::ButtonNormal;
    fillRect(btnX, btnY, btnSize, btnSize, muteColor);
    drawText(L"M", btnX + 5, btnY, DAWColors::TextPrimary, btnSize, btnSize);
    btnX += btnSize + btnSpacing;
    
    // Solo button  
    Color soloColor = track.isSolo() ? Color(0.9f, 0.8f, 0.2f) : DAWColors::ButtonNormal;
    fillRect(btnX, btnY, btnSize, btnSize, soloColor);
    drawText(L"S", btnX + 5, btnY, DAWColors::TextPrimary, btnSize, btnSize);
    btnX += btnSize + btnSpacing;
    
    // Arm button
    Color armColor = track.isArmed() ? Color(0.9f, 0.2f, 0.2f) : DAWColors::ButtonNormal;
    fillRect(btnX, btnY, btnSize, btnSize, armColor);
    drawText(L"R", btnX + 5, btnY, DAWColors::TextPrimary, btnSize, btnSize);
    
    // Volume indicator (simple bar)
    float volBarX = TRACK_HEADER_WIDTH - 30;
    float volBarH = height - 40;
    float volBarW = 8;
    float volBarY = y + 30;
    
    fillRect(volBarX, volBarY, volBarW, volBarH, DAWColors::ButtonNormal);
    float volFill = volBarH * track.getVolume();
    fillRect(volBarX, volBarY + volBarH - volFill, volBarW, volFill, 
             Color(0.4f, 0.8f, 0.4f));
    
    // Border
    drawLine(TRACK_HEADER_WIDTH - 1, y, TRACK_HEADER_WIDTH - 1, y + height,
             DAWColors::GridLine, 1.0f);
}

void TimelineView::drawTrackContent(ID2D1RenderTarget* rt, Track& track,
                                     float y, float height) {
    Color trackColor(track.getColor());
    
    for (const auto& region : track.getRegions()) {
        drawWaveform(rt, region, y, height, trackColor);
    }
}

void TimelineView::drawWaveform(ID2D1RenderTarget* rt, const TrackRegion& region,
                                 float trackY, float trackHeight, const Color& color) {
    if (!region.clip) return;
    
    int startX = timeToPixel(region.startTime);
    int endX = timeToPixel(region.startTime + region.duration);
    
    // Clip to visible area
    int visibleStart = std::max(startX, TRACK_HEADER_WIDTH);
    int visibleEnd = std::min(endX, getWidth());
    
    if (visibleEnd <= visibleStart) return;
    
    // Region background
    float regionY = trackY + 4;
    float regionHeight = trackHeight - 8;
    
    fillRect(static_cast<float>(startX), regionY, 
             static_cast<float>(endX - startX), regionHeight,
             Color(color.r * 0.3f, color.g * 0.3f, color.b * 0.3f, 0.8f));
    
    // Region border
    drawRect(static_cast<float>(startX), regionY,
             static_cast<float>(endX - startX), regionHeight,
             color, 1.0f);
    
    // Calculate the time range we need to display
    // visibleStart/visibleEnd are in pixels, convert to time within the clip
    double visibleStartTime = pixelToTime(visibleStart);
    double visibleEndTime = pixelToTime(visibleEnd);
    
    // Convert to time relative to clip start (accounting for region offset)
    double clipStartTime = (visibleStartTime - region.startTime) + region.clipOffset;
    double clipEndTime = (visibleEndTime - region.startTime) + region.clipOffset;
    
    // Clamp to valid clip range
    clipStartTime = std::max(0.0, clipStartTime);
    clipEndTime = std::min(region.clip->getDuration(), clipEndTime);
    
    if (clipEndTime <= clipStartTime) return;
    
    // Get waveform data for the visible portion only
    int waveformWidth = visibleEnd - visibleStart;
    auto waveform = region.clip->getWaveformData(waveformWidth, clipStartTime, clipEndTime);
    
    if (waveform.empty()) return;
    
    // Draw waveform
    float centerY = regionY + regionHeight / 2;
    float amplitude = (regionHeight / 2) * 0.9f;
    
    for (int x = 0; x < waveformWidth && x < static_cast<int>(waveform.size()); ++x) {
        auto [minVal, maxVal] = waveform[x];
        
        float y1 = centerY - maxVal * amplitude;
        float y2 = centerY - minVal * amplitude;
        
        drawLine(static_cast<float>(visibleStart + x), y1,
                 static_cast<float>(visibleStart + x), y2,
                 DAWColors::Waveform, 1.0f);
    }
}

void TimelineView::drawPlayhead(ID2D1RenderTarget* rt) {
    int x = timeToPixel(m_playheadPosition);
    
    if (x >= TRACK_HEADER_WIDTH && x <= getWidth()) {
        // Playhead line
        drawLine(static_cast<float>(x), 0, 
                 static_cast<float>(x), static_cast<float>(getHeight()),
                 DAWColors::Playhead, 2.0f);
        
        // Playhead triangle at top
        ID2D1PathGeometry* geometry = nullptr;
        Application::getInstance().getD2DFactory()->CreatePathGeometry(&geometry);
        
        if (geometry) {
            ID2D1GeometrySink* sink = nullptr;
            geometry->Open(&sink);
            
            if (sink) {
                float triSize = 8;
                sink->BeginFigure(D2D1::Point2F(static_cast<float>(x), RULER_HEIGHT),
                                  D2D1_FIGURE_BEGIN_FILLED);
                sink->AddLine(D2D1::Point2F(static_cast<float>(x - triSize), 0));
                sink->AddLine(D2D1::Point2F(static_cast<float>(x + triSize), 0));
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                sink->Release();
                
                getBrush()->SetColor(DAWColors::Playhead.toD2D());
                rt->FillGeometry(geometry, getBrush());
            }
            geometry->Release();
        }
    }
}

double TimelineView::pixelToTime(int x) const {
    return (x - TRACK_HEADER_WIDTH) / m_pixelsPerSecond + m_scrollX;
}

int TimelineView::timeToPixel(double time) const {
    return static_cast<int>((time - m_scrollX) * m_pixelsPerSecond) + TRACK_HEADER_WIDTH;
}

double TimelineView::snapTime(double time) const {
    if (!m_snapToGrid) return time;
    
    double secondsPerBeat = 60.0 / m_bpm;
    return round(time / secondsPerBeat) * secondsPerBeat;
}

int TimelineView::getTrackAtY(int y) const {
    if (y < RULER_HEIGHT) return -1;
    
    int trackY = RULER_HEIGHT - m_scrollY;
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        // Skip tracks that aren't visible
        if (!m_tracks[i]->isVisible()) continue;
        
        int trackHeight = m_tracks[i]->getHeight();
        if (y >= trackY && y < trackY + trackHeight) {
            return static_cast<int>(i);
        }
        trackY += trackHeight;
    }
    return -1;
}

int TimelineView::getTrackYPosition(int trackIndex) const {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) return -1;
    
    int trackY = RULER_HEIGHT - m_scrollY;
    for (int i = 0; i < trackIndex; ++i) {
        // Skip non-visible tracks
        if (!m_tracks[i]->isVisible()) continue;
        trackY += m_tracks[i]->getHeight();
    }
    return trackY;
}

TimelineView::TrackButton TimelineView::getButtonAtPosition(int trackIndex, int x, int y) const {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
        return TrackButton::None;
    }
    
    // Button layout constants (must match drawTrackHeader)
    constexpr float btnSize = 20;
    constexpr float btnSpacing = 4;
    constexpr float btnStartX = 12;
    
    int trackY = getTrackYPosition(trackIndex);
    int trackHeight = m_tracks[trackIndex]->getHeight();
    int btnY = trackY + trackHeight - 28;
    
    // Check if y is in button row
    if (y < btnY || y > btnY + btnSize) {
        return TrackButton::None;
    }
    
    // Check which button (if any)
    float muteX = btnStartX;
    float soloX = muteX + btnSize + btnSpacing;
    float armX = soloX + btnSize + btnSpacing;
    
    if (x >= muteX && x < muteX + btnSize) {
        return TrackButton::Mute;
    }
    if (x >= soloX && x < soloX + btnSize) {
        return TrackButton::Solo;
    }
    if (x >= armX && x < armX + btnSize) {
        return TrackButton::Arm;
    }
    
    return TrackButton::None;
}

void TimelineView::onResize(int width, int height) {
    invalidate();
}

void TimelineView::onMouseDown(int x, int y, int button) {
    if (button == 0) {
        if (y < RULER_HEIGHT && x >= TRACK_HEADER_WIDTH) {
            // Click on ruler - move playhead
            m_draggingPlayhead = true;
            double time = pixelToTime(x);
            setPlayheadPosition(snapTime(time));
            if (m_onPlayheadChanged) {
                m_onPlayheadChanged(m_playheadPosition);
            }
        }
        else if (x < TRACK_HEADER_WIDTH) {
            // Click on track header
            int track = getTrackAtY(y);
            if (track >= 0) {
                m_selectedTrack = track;
                
                // Check for button clicks
                TrackButton btn = getButtonAtPosition(track, x, y);
                switch (btn) {
                    case TrackButton::Mute:
                        m_tracks[track]->setMuted(!m_tracks[track]->isMuted());
                        break;
                    case TrackButton::Solo:
                        m_tracks[track]->setSolo(!m_tracks[track]->isSolo());
                        break;
                    case TrackButton::Arm:
                        m_tracks[track]->setArmed(!m_tracks[track]->isArmed());
                        break;
                    default:
                        break;
                }
            }
        }
        else {
            // Click on track content area
            m_selectedTrack = getTrackAtY(y);
        }
    }
    
    m_dragStartX = x;
    m_dragStartY = y;
    invalidate();
}

void TimelineView::onMouseUp(int x, int y, int button) {
    m_draggingPlayhead = false;
    m_draggingRegion = false;
    invalidate();
}

void TimelineView::onMouseMove(int x, int y) {
    if (m_draggingPlayhead) {
        double time = pixelToTime(x);
        setPlayheadPosition(snapTime(time));
        if (m_onPlayheadChanged) {
            m_onPlayheadChanged(m_playheadPosition);
        }
    }
}

void TimelineView::onMouseWheel(int x, int y, int delta) {
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        // Ctrl + wheel = zoom
        double zoomFactor = (delta > 0) ? 1.2 : 0.8;
        double mouseTime = pixelToTime(x);
        
        m_pixelsPerSecond = std::clamp(m_pixelsPerSecond * zoomFactor, 10.0, 1000.0);
        
        // Keep mouse position at same time after zoom
        m_scrollX = mouseTime - (x - TRACK_HEADER_WIDTH) / m_pixelsPerSecond;
        m_scrollX = std::max(0.0, m_scrollX);
    }
    else {
        // Normal wheel = scroll
        if (GetKeyState(VK_SHIFT) & 0x8000) {
            // Horizontal scroll - disable follow mode when user manually scrolls
            m_followPlayhead = false;
            m_scrollX -= delta / 120.0 * 50 / m_pixelsPerSecond;
            m_scrollX = std::max(0.0, m_scrollX);
        }
        else {
            // Vertical scroll
            m_scrollY -= delta / 120 * 30;
            m_scrollY = std::max(0, m_scrollY);
        }
    }
    invalidate();
}

void TimelineView::onDoubleClick(int x, int y, int button) {
    if (button != 0) return;
    
    // Check if double-click is in track header area (on track name)
    if (x < TRACK_HEADER_WIDTH) {
        int trackIndex = getTrackAtY(y);
        if (trackIndex >= 0) {
            // Check if click is in the name area (top portion of track header)
            int trackY = RULER_HEIGHT - m_scrollY;
            for (int i = 0; i < trackIndex; ++i) {
                // Skip non-visible tracks when calculating Y position
                if (!m_tracks[i]->isVisible()) continue;
                trackY += m_tracks[i]->getHeight();
            }
            
            // Name area is roughly the top 30 pixels of the track header
            int nameAreaBottom = trackY + 30;
            if (y >= trackY && y < nameAreaBottom) {
                startTrackNameEdit(trackIndex);
            }
        }
    }
}

void TimelineView::startTrackNameEdit(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) return;
    
    // Don't allow editing non-visible tracks
    if (!m_tracks[trackIndex]->isVisible()) return;
    
    // Cancel any existing edit
    if (m_editControl) {
        cancelTrackNameEdit();
    }
    
    m_editingTrackIndex = trackIndex;
    auto& track = m_tracks[trackIndex];
    
    // Calculate position for edit control (skip non-visible tracks)
    int trackY = RULER_HEIGHT - m_scrollY;
    for (int i = 0; i < trackIndex; ++i) {
        // Skip non-visible tracks when calculating Y position
        if (!m_tracks[i]->isVisible()) continue;
        trackY += m_tracks[i]->getHeight();
    }
    
    // Edit control position (in client coordinates)
    int editX = 12;
    int editY = trackY + 6;
    int editWidth = TRACK_HEADER_WIDTH - 45;
    int editHeight = 20;
    
    // Create edit control
    m_editControl = CreateWindowEx(
        0,
        L"EDIT",
        track->getName().c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        editX, editY, editWidth, editHeight,
        getHWND(),
        nullptr,
        Application::getInstance().getHInstance(),
        nullptr
    );
    
    if (m_editControl) {
        // Create font if not already created
        if (!m_editFont) {
            m_editFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        }
        SendMessage(m_editControl, WM_SETFONT, (WPARAM)m_editFont, TRUE);
        
        // Select all text
        SendMessage(m_editControl, EM_SETSEL, 0, -1);
        
        // Subclass the edit control to handle Enter/Escape keys
        SetWindowSubclass(m_editControl, EditSubclassProc, 0, reinterpret_cast<DWORD_PTR>(this));
        
        // Focus the edit control
        SetFocus(m_editControl);
    }
}

void TimelineView::commitTrackNameEdit() {
    if (!m_editControl || m_editingTrackIndex < 0) return;
    
    // Get text from edit control
    int length = GetWindowTextLength(m_editControl);
    std::wstring newName(length + 1, L'\0');
    GetWindowText(m_editControl, &newName[0], length + 1);
    newName.resize(length);
    
    // Update track name if not empty
    if (!newName.empty() && m_editingTrackIndex < static_cast<int>(m_tracks.size())) {
        m_tracks[m_editingTrackIndex]->setName(newName);
    }
    
    // Clean up
    RemoveWindowSubclass(m_editControl, EditSubclassProc, 0);
    DestroyWindow(m_editControl);
    m_editControl = nullptr;
    m_editingTrackIndex = -1;
    
    SetFocus(getHWND());
    invalidate();
}

void TimelineView::cancelTrackNameEdit() {
    if (!m_editControl) return;
    
    // Clean up without saving
    RemoveWindowSubclass(m_editControl, EditSubclassProc, 0);
    DestroyWindow(m_editControl);
    m_editControl = nullptr;
    m_editingTrackIndex = -1;
    
    SetFocus(getHWND());
    invalidate();
}

LRESULT CALLBACK TimelineView::EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam,
                                                  LPARAM lParam, UINT_PTR uIdSubclass,
                                                  DWORD_PTR dwRefData) {
    TimelineView* view = reinterpret_cast<TimelineView*>(dwRefData);
    
    switch (msg) {
    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            view->commitTrackNameEdit();
            return 0;
        }
        else if (wParam == VK_ESCAPE) {
            view->cancelTrackNameEdit();
            return 0;
        }
        break;
        
    case WM_KILLFOCUS:
        // Commit when focus is lost (clicking elsewhere)
        view->commitTrackNameEdit();
        return 0;
        
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, EditSubclassProc, uIdSubclass);
        break;
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}
