#pragma once
#include "D2DWindow.h"
#include "TooltipWindow.h"
#include <functional>

class TransportBar : public D2DWindow {
public:
    using Callback = std::function<void()>;
    using PositionCallback = std::function<void(double)>;

    ~TransportBar() { releaseGeometries(); }

    void setFollowPlayheadCallback(Callback cb) { m_onFollowPlayhead = cb; }
    void setPlayCallback(Callback cb) { m_onPlay = cb; }
    void setStopCallback(Callback cb) { m_onStop = cb; }
    void setPauseCallback(Callback cb) { m_onPause = cb; }
    void setRewindCallback(Callback cb) { m_onRewind = cb; }
    void setFastForwardCallback(Callback cb) { m_onFastForward = cb; }
    void setRecordCallback(Callback cb) { m_onRecord = cb; }

    void setPlaying(bool playing) { if (m_isPlaying != playing) { m_isPlaying = playing; invalidate(); } }
    void setRecording(bool recording) { if (m_isRecording != recording) { m_isRecording = recording; invalidate(); } }
    void setFollowingPlayhead(bool following) { if (m_isFollowingPlayhead != following) { m_isFollowingPlayhead = following; invalidate(); } }
    bool isRecording() const { return m_isRecording; }
    void setPosition(double seconds);
    void setDuration(double seconds);
    void setBPM(double bpm) { if (m_bpm != bpm) { m_bpm = bpm; invalidate(); } }
    void setHasAudioLoaded(bool loaded) { m_hasAudioLoaded = loaded; }

protected:
    void onRender(ID2D1RenderTarget* rt) override;
    void onResize(int width, int height) override;
    void onMouseDown(int x, int y, int button) override;
    void onMouseUp(int x, int y, int button) override;
    void onMouseMove(int x, int y) override;
    void onTimer(UINT_PTR timerId) override;

private:
    struct Button {
        float x, y, w, h;
        enum Type { FollowPlayhead, Play, Stop, Pause, Rewind, FastForward, Record } type;
        bool hovered = false;
        bool pressed = false;
        std::wstring tooltip;
    };

    void layoutButtons();
    void drawButton(ID2D1RenderTarget* rt, const Button& btn);
    void drawFollowPlayheadIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);
    void drawPlayIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);
    void drawPauseIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);
    void drawStopIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);
    void drawRewindIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);
    void drawFastForwardIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);
    void drawRecordIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);

    void initializeGeometries();
    void releaseGeometries();
    std::wstring formatTime(double seconds);
    std::wstring getTooltipForButton(Button::Type type) const;
    void updateTooltip();

    std::vector<Button> m_buttons;
    bool m_buttonsInitialized = false;

    // Cached geometries for performance
    ID2D1PathGeometry* m_playGeometry = nullptr;
    ID2D1PathGeometry* m_rewindGeometry = nullptr;
    ID2D1PathGeometry* m_fastForwardGeometry = nullptr;

    // Cached strings for performance
    std::wstring m_cachedPositionStr;
    std::wstring m_cachedDurationStr;
    std::wstring m_cachedBpmStr;
    double m_cachedPosition = -1.0;
    double m_cachedDuration = -1.0;
    double m_cachedBpm = -1.0;
    int m_lastWidth = 0;
    int m_lastHeight = 0;

    bool m_isPlaying = false;
    bool m_isRecording = false;
    bool m_isFollowingPlayhead = true;
    double m_position = 0.0;
    double m_duration = 0.0;
    double m_bpm = 120.0;

    Callback m_onFollowPlayhead;
    Callback m_onPlay;
    Callback m_onStop;
    Callback m_onPause;
    Callback m_onRewind;
    Callback m_onFastForward;
    Callback m_onRecord;

    bool m_hasAudioLoaded = false;  // Tracks if audio is loaded in the project

    // Tooltip state
    TooltipWindow m_tooltip;
    static const UINT_PTR TOOLTIP_TIMER_ID = 1001;
    int m_tooltipButtonIndex = -1;
    bool m_showTooltip = false;
};
