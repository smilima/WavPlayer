#pragma once
#include "D2DWindow.h"
#include "Track.h"
#include <vector>
#include <memory>
#include <functional>

class TimelineView : public D2DWindow {
public:
    using PlayheadCallback = std::function<void(double)>;
    using RegionChangedCallback = std::function<void()>;

    ~TimelineView();

    void addTrack(std::shared_ptr<Track> track);
    void removeTrack(size_t index);
    const std::vector<std::shared_ptr<Track>>& getTracks() const { return m_tracks; }

    // Check if any track is armed for recording
    bool hasArmedTrack() const;

    // Get first armed track (for recording)
    std::shared_ptr<Track> getFirstArmedTrack() const;

    // Playhead
    void setPlayheadPosition(double seconds);
    double getPlayheadPosition() const { return m_playheadPosition; }
    void setPlayheadCallback(PlayheadCallback cb) { m_onPlayheadChanged = cb; }

    // Notify host when regions are added/removed/changed
    void setRegionChangedCallback(RegionChangedCallback cb) { m_onRegionChanged = std::move(cb); }

    // Timeline extent (seconds)
    void setTimelineDuration(double duration);

    // Follow playhead - auto-scroll to keep playhead visible during playback
    void setFollowPlayhead(bool follow) { m_followPlayhead = follow; }
    bool getFollowPlayhead() const { return m_followPlayhead; }

    // View settings
    void setPixelsPerSecond(double pps);
    double getPixelsPerSecond() const { return m_pixelsPerSecond; }

    void setScrollX(double x);
    double getScrollX() const { return m_scrollX; }

    void setScrollY(int y);
    int getScrollY() const { return m_scrollY; }

    // Grid settings
    void setBPM(double bpm) { m_bpm = bpm; invalidate(); }
    void setSnapToGrid(bool snap) { m_snapToGrid = snap; }
    void setShowGrid(bool show) { m_showGrid = show; invalidate(); }

    // Selection
    int getSelectedTrackIndex() const { return m_selectedTrack; }
    void setSelectedTrackIndex(int index);
    int getSelectedRegionIndex() const { return m_selectedRegion; }
    void setSelectedRegion(int trackIndex, int regionIndex);
    void clearRegionSelection();

    // Track header width
    static constexpr int TRACK_HEADER_WIDTH = 200;
    static constexpr int RULER_HEIGHT = 30;

protected:
    void onRender(ID2D1RenderTarget* rt) override;
    void onResize(int width, int height) override;
    void onMouseDown(int x, int y, int button) override;
    void onMouseUp(int x, int y, int button) override;
    void onMouseMove(int x, int y) override;
    void onMouseWheel(int x, int y, int delta) override;
    void onDoubleClick(int x, int y, int button) override;
    void onHScroll(int request, int pos) override;

private:
    void drawRuler(ID2D1RenderTarget* rt);
    void drawGrid(ID2D1RenderTarget* rt);
    void drawTracks(ID2D1RenderTarget* rt);
    void drawTrackHeader(ID2D1RenderTarget* rt, Track& track, float y, float height, bool isSelected);
    void drawTrackContent(ID2D1RenderTarget* rt, Track& track, float y, float height, size_t trackIndex);
    void drawWaveform(ID2D1RenderTarget* rt, const TrackRegion& region,
        float trackY, float trackHeight, const Color& color, bool isSelected);
    void drawPlayhead(ID2D1RenderTarget* rt);

    double pixelToTime(int x) const;
    int timeToPixel(double time) const;
    double snapTime(double time) const;
    int getTrackAtY(int y) const;
    void ensurePlayheadVisible();

    // Track header button handling
    enum class TrackButton { None, Mute, Solo, Arm };
    TrackButton getButtonAtPosition(int trackIndex, int x, int y) const;
    int getTrackYPosition(int trackIndex) const;
    int hitTestRegion(int trackIndex, int x, int y) const;

    void showRegionContextMenu(int x, int y);
    bool deleteSelectedRegion();

    void updateScrollMetrics();
    void syncScrollBarPosition();
    double getMaxScrollX() const;

    // Track name editing
    void startTrackNameEdit(int trackIndex);
    void commitTrackNameEdit();
    void cancelTrackNameEdit();
    bool isEditingTrackName() const { return m_editingTrackIndex >= 0; }
    static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam,
        LPARAM lParam, UINT_PTR uIdSubclass,
        DWORD_PTR dwRefData);

    std::vector<std::shared_ptr<Track>> m_tracks;

    double m_playheadPosition = 0.0;
    double m_pixelsPerSecond = 100.0;
    double m_scrollX = 0.0;
    int m_scrollY = 0;
    double m_timelineDuration = 0.0;

    double m_bpm = 120.0;
    bool m_snapToGrid = true;
    bool m_showGrid = true;
    bool m_followPlayhead = true;  // Auto-scroll to follow playhead

    // Interaction state
    bool m_draggingPlayhead = false;
    bool m_draggingRegion = false;
    int m_selectedTrack = -1;
    int m_selectedRegion = -1;
    int m_dragStartX = 0;
    int m_dragStartY = 0;
    double m_dragStartTime = 0.0;

    // Track name editing
    HWND m_editControl = nullptr;
    HFONT m_editFont = nullptr;
    int m_editingTrackIndex = -1;

    PlayheadCallback m_onPlayheadChanged;
    RegionChangedCallback m_onRegionChanged;
};