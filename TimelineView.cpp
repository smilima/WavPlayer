#include "TimelineView.h"
#include "Application.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <CommCtrl.h>

#pragma comment(lib, "Comctl32.lib")

namespace {
constexpr float SCROLLBAR_HEIGHT_DIP = 16.0f;
constexpr float SCROLLBAR_PADDING = 2.0f;
constexpr float SCROLLBAR_MIN_THUMB = 24.0f;
}

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

        if (m_selectedTrack == static_cast<int>(index)) {
            m_selectedTrack = -1;
            m_selectedRegion = -1;
        }
        else if (m_selectedTrack > static_cast<int>(index)) {
            --m_selectedTrack;
        }

        invalidate();
    }
}

void TimelineView::setTimelineDuration(double duration) {
    m_timelineDuration = std::max(0.0, duration);
    double clamped = std::min(m_scrollX, getMaxScrollX());
    if (std::abs(clamped - m_scrollX) > 1e-6) {
        m_scrollX = clamped;
        invalidate();
    }
    updateScrollMetrics();
}

void TimelineView::setSelectedTrackIndex(int index) {
    int clampedIndex = (index >= 0 && index < static_cast<int>(m_tracks.size())) ? index : -1;

    if (clampedIndex != m_selectedTrack) {
        m_selectedTrack = clampedIndex;
        if (clampedIndex == -1) {
            m_selectedRegion = -1;
        }
        invalidate();
    }
    else if (clampedIndex >= 0) {
        const auto& regions = m_tracks[clampedIndex]->getRegions();
        if (m_selectedRegion >= static_cast<int>(regions.size())) {
            m_selectedRegion = -1;
            invalidate();
        }
    }
}

void TimelineView::setSelectedRegion(int trackIndex, int regionIndex) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
        if (m_selectedTrack != -1 || m_selectedRegion != -1) {
            m_selectedTrack = -1;
            m_selectedRegion = -1;
            invalidate();
        }
        return;
    }

    const auto& regions = m_tracks[trackIndex]->getRegions();
    if (regionIndex < 0 || regionIndex >= static_cast<int>(regions.size())) {
        if (m_selectedTrack != trackIndex || m_selectedRegion != -1) {
            m_selectedTrack = trackIndex;
            m_selectedRegion = -1;
            invalidate();
        }
        return;
    }

    if (m_selectedTrack != trackIndex || m_selectedRegion != regionIndex) {
        m_selectedTrack = trackIndex;
        m_selectedRegion = regionIndex;
        invalidate();
    }
}

void TimelineView::clearRegionSelection() {
    if (m_selectedRegion != -1) {
        m_selectedRegion = -1;
        invalidate();
    }
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
        setScrollX(targetScrollX);
    }
    else if (playheadPixel < leftMargin && m_scrollX > 0) {
        // Playhead went past left edge (e.g., when rewinding) - scroll to show it
        double targetScrollX = m_playheadPosition - (contentWidth * 0.25) / m_pixelsPerSecond;
        setScrollX(targetScrollX);
    }
}

void TimelineView::setPixelsPerSecond(double pps) {
    m_pixelsPerSecond = std::clamp(pps, 10.0, 1000.0);
    setScrollX(m_scrollX);
    updateScrollMetrics();
    invalidate();
}

void TimelineView::setScrollX(double x) {
    double maxScroll = getMaxScrollX();
    double clamped = std::clamp(x, 0.0, maxScroll);
    bool changed = std::abs(clamped - m_scrollX) > 1e-6;
    m_scrollX = clamped;
    if (changed) {
        invalidate();
    }
}

void TimelineView::setScrollY(int y) {
    m_scrollY = std::max(0, y);
    invalidate();
}

void TimelineView::updateScrollMetrics() {
    const int viewWidthDip = std::max(0, getWidth() - TRACK_HEADER_WIDTH);
    const double contentWidthDip = m_timelineDuration * m_pixelsPerSecond;
    const bool shouldShow = (viewWidthDip > 0) && (contentWidthDip > static_cast<double>(viewWidthDip));

    if (m_scrollbarVisible != shouldShow) {
        m_scrollbarVisible = shouldShow;
        invalidate();
    }

    m_scrollbarHeightDip = shouldShow ? SCROLLBAR_HEIGHT_DIP : 0.0f;
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
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        auto& track = m_tracks[i];

        // Skip tracks that aren't visible
        if (!track->isVisible()) continue;

        if (y + track->getHeight() > RULER_HEIGHT && y < getVisualHeight()) {
            bool isSelected = (static_cast<int>(i) == m_selectedTrack);
            drawTrackHeader(rt, *track, y, static_cast<float>(track->getHeight()), isSelected);
        }
        y += track->getHeight();
    }

    // Draw custom scrollbar on top of content
    drawScrollbar(rt);

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

    float gridBottom = static_cast<float>(getVisualHeight());

    for (double t = startTime; t <= endTime; t += gridInterval) {
        int x = timeToPixel(t);
        if (x < TRACK_HEADER_WIDTH || x > getWidth()) continue;

        // Check if this is a bar line
        bool isBarLine = fmod(t, secondsPerBar) < 0.001;

        drawLine(static_cast<float>(x), static_cast<float>(RULER_HEIGHT),
            static_cast<float>(x), gridBottom,
            isBarLine ? DAWColors::GridLineMajor : DAWColors::GridLine,
            isBarLine ? 1.0f : 0.5f);
    }
}

void TimelineView::drawTracks(ID2D1RenderTarget* rt) {
    float y = static_cast<float>(RULER_HEIGHT - m_scrollY);
    int visibleTrackIndex = 0;  // Count only visible tracks for alternating colors
    int viewBottom = getVisualHeight();

    for (size_t i = 0; i < m_tracks.size(); ++i) {
        auto& track = m_tracks[i];

        // Skip tracks that aren't visible
        if (!track->isVisible()) continue;

        float height = static_cast<float>(track->getHeight());

        if (y + height > RULER_HEIGHT && y < viewBottom) {
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
            drawTrackContent(rt, *track, y, height, i);

            // Track separator line
            drawLine(0, y + height, static_cast<float>(getWidth()), y + height,
                DAWColors::GridLine, 1.0f);
        }

        y += height;
        ++visibleTrackIndex;
    }
}

void TimelineView::drawTrackHeader(ID2D1RenderTarget* rt, Track& track,
    float y, float height, bool isSelected) {
    // Header background - highlight if selected
    Color headerColor = isSelected ?
        Color(DAWColors::TrackHeader.r * 1.5f, DAWColors::TrackHeader.g * 1.5f, DAWColors::TrackHeader.b * 1.5f) :
        DAWColors::TrackHeader;

    fillRect(0, y, TRACK_HEADER_WIDTH, height, headerColor);

    // Draw selection border if selected
    if (isSelected) {
        drawRect(1, y + 1, TRACK_HEADER_WIDTH - 2, height - 2, DAWColors::Selection, 2.0f);
    }

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
    float y, float height, size_t trackIndex) {
    Color trackColor(track.getColor());
    const auto& regions = track.getRegions();

    for (size_t regionIndex = 0; regionIndex < regions.size(); ++regionIndex) {
        bool isSelected = (static_cast<int>(trackIndex) == m_selectedTrack) &&
            (static_cast<int>(regionIndex) == m_selectedRegion);
        drawWaveform(rt, regions[regionIndex], y, height, trackColor, isSelected);
    }
}

void TimelineView::drawWaveform(ID2D1RenderTarget* rt, const TrackRegion& region,
    float trackY, float trackHeight, const Color& color, bool isSelected) {
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

    if (isSelected) {
        Color overlay(DAWColors::Selection.r, DAWColors::Selection.g, DAWColors::Selection.b, 0.35f);
        fillRect(static_cast<float>(startX), regionY,
            static_cast<float>(endX - startX), regionHeight,
            overlay);
    }

    Color borderColor = isSelected ? DAWColors::Selection : color;
    float borderWidth = isSelected ? 2.0f : 1.0f;

    drawRect(static_cast<float>(startX), regionY,
        static_cast<float>(endX - startX), regionHeight,
        borderColor, borderWidth);

    // Calculate the time range we need to display
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
        float bottom = static_cast<float>(getVisualHeight());
        // Playhead line
        drawLine(static_cast<float>(x), 0,
            static_cast<float>(x), bottom,
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

void TimelineView::drawScrollbar(ID2D1RenderTarget* rt) {
    if (!m_scrollbarVisible) {
        return;
    }

    ScrollbarMetrics metrics;
    if (!computeScrollbarMetrics(metrics)) {
        return;
    }

    // Fill area under the track headers for a seamless look
    fillRect(0, metrics.barY, TRACK_HEADER_WIDTH, metrics.barHeight, DAWColors::TrackHeader);

    Color grooveColor(
        DAWColors::TrackBackground.r * 0.7f,
        DAWColors::TrackBackground.g * 0.7f,
        DAWColors::TrackBackground.b * 0.7f,
        1.0f);
    Color grooveBorder(
        DAWColors::GridLine.r,
        DAWColors::GridLine.g,
        DAWColors::GridLine.b,
        0.8f);

    fillRect(metrics.barX, metrics.barY, metrics.barWidth, metrics.barHeight, grooveColor);
    drawRect(metrics.barX, metrics.barY, metrics.barWidth, metrics.barHeight, grooveBorder, 1.0f);

    Color thumbColor = m_draggingScrollbar ? DAWColors::Selection : DAWColors::ButtonNormal;
    fillRect(metrics.thumbX,
        metrics.barY + SCROLLBAR_PADDING,
        metrics.thumbWidth,
        metrics.barHeight - SCROLLBAR_PADDING * 2,
        thumbColor);
    drawRect(metrics.thumbX,
        metrics.barY + SCROLLBAR_PADDING,
        metrics.thumbWidth,
        metrics.barHeight - SCROLLBAR_PADDING * 2,
        DAWColors::GridLine, 1.0f);
}

void TimelineView::onHScroll(HWND /*scrollBar*/, int /*request*/, int /*pos*/) {
    // Custom scrollbar uses mouse handling; no-op for WM_HSCROLL
}

double TimelineView::getMaxScrollX() const {
    int viewWidth = std::max(0, getWidth() - TRACK_HEADER_WIDTH);
    if (viewWidth <= 0 || m_pixelsPerSecond <= 0.0) {
        return 0.0;
    }
    double visibleSeconds = static_cast<double>(viewWidth) / m_pixelsPerSecond;
    return std::max(0.0, m_timelineDuration - visibleSeconds);
}

int TimelineView::getVisualHeight() const {
    int height = getHeight();
    if (m_scrollbarVisible) {
        height -= static_cast<int>(std::ceil(m_scrollbarHeightDip));
    }
    return std::max(height, RULER_HEIGHT);
}

bool TimelineView::hasArmedTrack() const {
    return std::any_of(m_tracks.begin(), m_tracks.end(),
        [](const std::shared_ptr<Track>& track) {
            return track && track->isVisible() && track->isArmed();
        });
}

std::shared_ptr<Track> TimelineView::getFirstArmedTrack() const {
    auto it = std::find_if(m_tracks.begin(), m_tracks.end(),
        [](const std::shared_ptr<Track>& track) {
            return track && track->isVisible() && track->isArmed();
        });
    return (it != m_tracks.end()) ? *it : nullptr;
}

double TimelineView::snapTime(double time) const {
    if (!m_snapToGrid) return time;

    double secondsPerBeat = 60.0 / m_bpm;
    return round(time / secondsPerBeat) * secondsPerBeat;
}

int TimelineView::getTrackAtY(int y) const {
    if (y < RULER_HEIGHT) return -1;
    if (y >= getVisualHeight()) return -1;

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

bool TimelineView::computeScrollbarMetrics(ScrollbarMetrics& metrics) const {
    if (!m_scrollbarVisible) {
        return false;
    }

    const int viewWidthDip = std::max(0, getWidth() - TRACK_HEADER_WIDTH);
    if (viewWidthDip <= 0 || m_pixelsPerSecond <= 0.0) {
        return false;
    }

    metrics.barX = static_cast<float>(TRACK_HEADER_WIDTH);
    metrics.barWidth = static_cast<float>(viewWidthDip);
    metrics.barHeight = m_scrollbarHeightDip;
    metrics.barY = static_cast<float>(getHeight()) - metrics.barHeight;

    const double contentWidthDip = std::max(1.0, m_timelineDuration * m_pixelsPerSecond);
    const double thumbRatio = std::clamp(static_cast<double>(viewWidthDip) / contentWidthDip, 0.0, 1.0);
    metrics.thumbWidth = std::max(SCROLLBAR_MIN_THUMB, metrics.barWidth * static_cast<float>(thumbRatio));

    const double maxScroll = getMaxScrollX();
    const float travel = std::max(0.0f, metrics.barWidth - metrics.thumbWidth);
    const double scrollRatio = (maxScroll <= 0.0) ? 0.0 : (m_scrollX / maxScroll);
    metrics.thumbX = metrics.barX + static_cast<float>(travel * scrollRatio);
    return true;
}

void TimelineView::onResize(int /*width*/, int /*height*/) {
    setScrollX(m_scrollX);  // Clamp to new bounds
    updateScrollMetrics();
    invalidate();
}

double TimelineView::pixelToTime(int x) const {
    if (m_pixelsPerSecond <= 0.0) {
        return 0.0;
    }

    int relative = std::max(0, x - TRACK_HEADER_WIDTH);
    double time = m_scrollX + static_cast<double>(relative) / m_pixelsPerSecond;
    return std::clamp(time, 0.0, std::max(m_timelineDuration, 0.0));
}

int TimelineView::timeToPixel(double time) const {
    double clamped = std::max(0.0, time);
    double relative = (clamped - m_scrollX) * m_pixelsPerSecond;
    return TRACK_HEADER_WIDTH + static_cast<int>(std::round(relative));
}

TimelineView::TrackButton TimelineView::getButtonAtPosition(int trackIndex, int x, int y) const {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
        return TrackButton::None;
    }

    int trackY = getTrackYPosition(trackIndex);
    if (trackY < 0) {
        return TrackButton::None;
    }

    auto& track = m_tracks[trackIndex];
    float trackHeight = static_cast<float>(track->getHeight());

    // Button layout from drawTrackHeader
    float btnSize = 20;
    float btnSpacing = 4;
    float btnY = trackY + trackHeight - 28;
    float btnX = 12;

    // Check if Y coordinate is within button area
    if (y < btnY || y > btnY + btnSize) {
        return TrackButton::None;
    }

    // Check Mute button
    if (x >= btnX && x < btnX + btnSize) {
        return TrackButton::Mute;
    }
    btnX += btnSize + btnSpacing;

    // Check Solo button
    if (x >= btnX && x < btnX + btnSize) {
        return TrackButton::Solo;
    }
    btnX += btnSize + btnSpacing;

    // Check Arm button
    if (x >= btnX && x < btnX + btnSize) {
        return TrackButton::Arm;
    }

    return TrackButton::None;
}

void TimelineView::onMouseDown(int x, int y, int button) {
    // Handle right-click for context menu
    if (button == 1) {
        int trackIndex = getTrackAtY(y);
        if (trackIndex >= 0) {
            setSelectedTrackIndex(trackIndex);
            showTrackContextMenu(x, y);
        }
        return;
    }

    if (button != 0) {
        return;
    }

    m_dragStartX = x;
    m_dragStartY = y;

    // Scrollbar hit test first so it takes priority
    if (m_scrollbarVisible) {
        ScrollbarMetrics metrics;
        if (computeScrollbarMetrics(metrics)) {
            float barBottom = metrics.barY + metrics.barHeight;
            if (y >= metrics.barY && y <= barBottom) {
                if (x >= metrics.thumbX && x <= metrics.thumbX + metrics.thumbWidth) {
                    m_draggingScrollbar = true;
                    m_scrollbarDragOffset = static_cast<float>(x) - metrics.thumbX;
                    return;
                }
                else {
                    // Clicked the groove - page scroll left/right
                    double pageSeconds = (getWidth() - TRACK_HEADER_WIDTH) * 0.8 / std::max(m_pixelsPerSecond, 1.0);
                    if (x < metrics.thumbX) {
                        setScrollX(m_scrollX - pageSeconds);
                    }
                    else {
                        setScrollX(m_scrollX + pageSeconds);
                    }
                    return;
                }
            }
        }
    }

    if (y < RULER_HEIGHT) {
        setPlayheadPosition(pixelToTime(x));
        m_draggingPlayhead = true;
        m_dragStartTime = m_playheadPosition;
        return;
    }

    int trackIndex = getTrackAtY(y);
    if (trackIndex >= 0) {
        setSelectedTrackIndex(trackIndex);

        // Check if clicking on track header buttons
        if (x < TRACK_HEADER_WIDTH) {
            TrackButton btn = getButtonAtPosition(trackIndex, x, y);
            if (btn != TrackButton::None) {
                auto& track = m_tracks[trackIndex];
                switch (btn) {
                case TrackButton::Mute:
                    track->setMuted(!track->isMuted());
                    break;
                case TrackButton::Solo:
                    track->setSolo(!track->isSolo());
                    break;
                case TrackButton::Arm:
                    track->setArmed(!track->isArmed());
                    break;
                default:
                    break;
                }
                invalidate();
                return;
            }
        }

        if (x >= TRACK_HEADER_WIDTH) {
            double clickTime = pixelToTime(x);
            const auto& regions = m_tracks[trackIndex]->getRegions();
            int regionIndex = -1;
            for (size_t i = 0; i < regions.size(); ++i) {
                if (clickTime >= regions[i].startTime && clickTime <= regions[i].endTime()) {
                    regionIndex = static_cast<int>(i);
                    break;
                }
            }

            setSelectedRegion(trackIndex, regionIndex);
            if (regionIndex == -1) {
                m_draggingRegion = false;
            }
            else {
                m_draggingRegion = true;
                m_dragStartTime = clickTime;
            }
        }
    }
}

void TimelineView::onMouseUp(int /*x*/, int /*y*/, int button) {
    if (button != 0) {
        return;
    }

    m_draggingPlayhead = false;
    m_draggingRegion = false;
    m_draggingScrollbar = false;
}

void TimelineView::onMouseMove(int x, int /*y*/) {
    if (m_draggingPlayhead) {
        setPlayheadPosition(pixelToTime(x));
        return;
    }

    if (m_draggingScrollbar) {
        ScrollbarMetrics metrics;
        if (computeScrollbarMetrics(metrics)) {
            float minThumb = metrics.barX;
            float maxThumb = metrics.barX + metrics.barWidth - metrics.thumbWidth;
            float newThumbX = std::clamp(static_cast<float>(x) - m_scrollbarDragOffset, minThumb, maxThumb);
            float travel = metrics.barWidth - metrics.thumbWidth;
            double ratio = (travel <= 0.0f) ? 0.0 : (newThumbX - metrics.barX) / travel;
            double targetScroll = ratio * getMaxScrollX();
            setScrollX(targetScroll);
        }
        return;
    }
}

void TimelineView::onMouseWheel(int /*x*/, int /*y*/, int delta) {
    if (m_pixelsPerSecond <= 0.0) {
        return;
    }

    double wheelSteps = static_cast<double>(delta) / WHEEL_DELTA;
    double pagePixels = std::max(1, getWidth() - TRACK_HEADER_WIDTH);
    double secondsDelta = -(wheelSteps * pagePixels * 0.1) / m_pixelsPerSecond;
    setScrollX(m_scrollX + secondsDelta);
}

void TimelineView::onDoubleClick(int x, int y, int button) {
    if (button != 0) {
        return;
    }

    if (y < RULER_HEIGHT) {
        setPlayheadPosition(pixelToTime(x));
        return;
    }

    int trackIndex = getTrackAtY(y);
    if (trackIndex >= 0) {
        setSelectedTrackIndex(trackIndex);
        if (x >= TRACK_HEADER_WIDTH) {
            setPlayheadPosition(pixelToTime(x));
        }
    }
}

void TimelineView::showTrackContextMenu(int x, int y) {
    if (m_selectedTrack < 0 || m_selectedTrack >= static_cast<int>(m_tracks.size())) {
        return;
    }

    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    // Add menu items
    AppendMenu(menu, MF_STRING, 2001, L"Delete Track");

    // Convert DIP coordinates to physical pixels, then to screen coordinates
    POINT pt = {
        static_cast<LONG>(dipsToPixelsX(static_cast<float>(x))),
        static_cast<LONG>(dipsToPixelsY(static_cast<float>(y)))
    };
    ClientToScreen(getHWND(), &pt);

    // Show the context menu
    int result = TrackPopupMenu(
        menu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
        pt.x, pt.y,
        0,
        getHWND(),
        nullptr
    );

    DestroyMenu(menu);

    // Handle menu selection
    if (result == 2001) {
        deleteSelectedTrack();
    }
}

bool TimelineView::deleteSelectedTrack() {
    if (m_selectedTrack < 0 || m_selectedTrack >= static_cast<int>(m_tracks.size())) {
        return false;
    }

    // Notify parent to handle the actual deletion
    // The MainWindow will show confirmation dialog and handle cleanup
    if (m_onTrackDelete) {
        m_onTrackDelete();
    }

    return true;
}