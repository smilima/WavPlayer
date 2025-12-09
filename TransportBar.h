#pragma once
#include "D2DWindow.h"
#include <functional>

class TransportBar : public D2DWindow {
public:
    using Callback = std::function<void()>;
    using PositionCallback = std::function<void(double)>;

    void setPlayCallback(Callback cb) { m_onPlay = cb; }
    void setStopCallback(Callback cb) { m_onStop = cb; }
    void setPauseCallback(Callback cb) { m_onPause = cb; }
    void setRewindCallback(Callback cb) { m_onRewind = cb; }
    void setFastForwardCallback(Callback cb) { m_onFastForward = cb; }

    void setPlaying(bool playing) { m_isPlaying = playing; invalidate(); }
    void setPosition(double seconds);
    void setDuration(double seconds);
    void setBPM(double bpm) { m_bpm = bpm; invalidate(); }

protected:
    void onRender(ID2D1RenderTarget* rt) override;
    void onMouseDown(int x, int y, int button) override;
    void onMouseUp(int x, int y, int button) override;
    void onMouseMove(int x, int y) override;
    void onResize(int width, int height) override;

private:
    struct Button {
        float x, y, w, h;
        enum Type { Play, Stop, Pause, Rewind, FastForward } type;
        bool hovered = false;
        bool pressed = false;
    };

    void layoutButtons();
    void drawButton(ID2D1RenderTarget* rt, const Button& btn);
    void drawPlayIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);
    void drawPauseIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);
    void drawStopIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);
    void drawRewindIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);
    void drawFastForwardIcon(ID2D1RenderTarget* rt, float cx, float cy, float size);
    
    std::wstring formatTime(double seconds);

    std::vector<Button> m_buttons;
    bool m_buttonsInitialized = false;

    bool m_isPlaying = false;
    double m_position = 0.0;
    double m_duration = 0.0;
    double m_bpm = 120.0;

    Callback m_onPlay;
    Callback m_onStop;
    Callback m_onPause;
    Callback m_onRewind;
    Callback m_onFastForward;
};
