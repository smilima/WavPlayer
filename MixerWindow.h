#pragma once
#include "D2DWindow.h"
#include "Track.h"
#include <vector>
#include <memory>
#include <functional>

class MixerWindow : public D2DWindow {
public:
    MixerWindow();
    ~MixerWindow() override;

    // Set the tracks to display in the mixer
    void setTracks(std::vector<std::shared_ptr<Track>>* tracks);

    // Callback when mixer values change
    using ChangeCallback = std::function<void()>;
    void setChangeCallback(ChangeCallback callback) { m_changeCallback = callback; }

protected:
    void onRender(ID2D1RenderTarget* rt) override;
    void onResize(int width, int height) override;
    void onMouseDown(int x, int y, int button) override;
    void onMouseUp(int x, int y, int button) override;
    void onMouseMove(int x, int y) override;
    bool onClose() override { return true; }  // Hide instead of destroy

private:
    // UI element types
    enum class ControlType {
        None,
        VolumeSlider,
        PanKnob,
        LowEQKnob,
        MidEQKnob,
        HighEQKnob,
        MuteButton,
        SoloButton
    };

    struct Control {
        ControlType type;
        int trackIndex;
        float x, y, w, h;
        bool hovered = false;
    };

    // Helper functions
    void drawChannelStrip(ID2D1RenderTarget* rt, int trackIndex, float x, float y, float width, float height);
    void drawVUMeter(ID2D1RenderTarget* rt, float x, float y, float width, float height, float peakLevel);
    void drawVolumeSlider(ID2D1RenderTarget* rt, float x, float y, float width, float height, float volumeDB, bool hovered);
    void drawRotaryKnob(ID2D1RenderTarget* rt, float x, float y, float radius, float value, bool hovered, const wchar_t* label);
    void drawButton(ID2D1RenderTarget* rt, float x, float y, float width, float height, const wchar_t* text, bool active, bool hovered);

    Control* getControlAtPosition(int x, int y);
    float getValueFromSliderY(int y, float sliderY, float sliderHeight);
    float getValueFromKnobAngle(int mouseX, int mouseY, float knobCenterX, float knobCenterY);

    // dB conversion helpers
    static float linearToDB(float linear);
    static float dbToLinear(float db);
    static float getDBFromSliderY(int y, float sliderY, float sliderHeight);

    // Constants
    static constexpr float CHANNEL_WIDTH = 140.0f;  // Increased for VU meter
    static constexpr float CHANNEL_SPACING = 20.0f;
    static constexpr float MARGIN = 20.0f;
    static constexpr float KNOB_RADIUS = 20.0f;
    static constexpr float SLIDER_HEIGHT = 150.0f;
    static constexpr float SLIDER_WIDTH = 30.0f;
    static constexpr float VU_METER_WIDTH = 15.0f;
    static constexpr float BUTTON_HEIGHT = 30.0f;

    // dB scale range for volume slider
    static constexpr float MIN_DB = -60.0f;
    static constexpr float MAX_DB = 6.0f;

    // Data
    std::vector<std::shared_ptr<Track>>* m_tracks = nullptr;
    std::vector<Control> m_controls;
    Control* m_draggedControl = nullptr;
    ChangeCallback m_changeCallback;

    // Cached rendering resources
    ID2D1PathGeometry* m_knobTickGeometry = nullptr;
};
