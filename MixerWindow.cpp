#include "MixerWindow.h"
#include "Application.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

MixerWindow::MixerWindow() {
}

MixerWindow::~MixerWindow() {
    if (m_knobTickGeometry) {
        m_knobTickGeometry->Release();
    }
}

void MixerWindow::setTracks(std::vector<std::shared_ptr<Track>>* tracks) {
    m_tracks = tracks;
    invalidate();
}

// dB conversion helpers
float MixerWindow::linearToDB(float linear) {
    if (linear <= 0.0f) return MIN_DB;
    float db = 20.0f * log10f(linear);
    return std::max(MIN_DB, std::min(MAX_DB, db));
}

float MixerWindow::dbToLinear(float db) {
    if (db <= MIN_DB) return 0.0f;
    return powf(10.0f, db / 20.0f);
}

float MixerWindow::getDBFromSliderY(int y, float sliderY, float sliderHeight) {
    float relativeY = y - sliderY;
    float normalized = 1.0f - (relativeY / sliderHeight);
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    // Map 0-1 to MIN_DB to MAX_DB
    return MIN_DB + normalized * (MAX_DB - MIN_DB);
}

void MixerWindow::onRender(ID2D1RenderTarget* rt) {
    // Clear background
    fillRect(0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight()),
             DAWColors::Background);

    // Draw title
    drawText(L"Track Mixer", MARGIN, MARGIN, DAWColors::TextPrimary);

    if (!m_tracks || m_tracks->empty()) {
        drawText(L"No tracks", MARGIN, MARGIN + 30, DAWColors::TextSecondary);
        return;
    }

    // Clear controls for rebuilding
    m_controls.clear();

    // Calculate channel positions
    float y = MARGIN + 40;
    float height = static_cast<float>(getHeight()) - y - MARGIN;

    for (size_t i = 0; i < m_tracks->size(); i++) {
        float x = MARGIN + i * (CHANNEL_WIDTH + CHANNEL_SPACING);
        drawChannelStrip(rt, static_cast<int>(i), x, y, CHANNEL_WIDTH, height);
    }
}

void MixerWindow::drawChannelStrip(ID2D1RenderTarget* rt, int trackIndex, float x, float y, float width, float height) {
    auto track = (*m_tracks)[trackIndex];

    // Draw background panel
    fillRect(x, y, width, height, DAWColors::TrackBackground);
    drawRect(x, y, width, height, DAWColors::GridLine, 1.0f);

    // Draw track name at top
    drawText(track->getName(), x + 5, y + 5, DAWColors::TextPrimary, width - 10);

    float currentY = y + 30;

    // VU meter on the left
    float vuMeterX = x + 5;
    float vuMeterY = currentY;
    drawVUMeter(rt, vuMeterX, vuMeterY, VU_METER_WIDTH, SLIDER_HEIGHT, track->getPeakLevel());

    // Volume slider (center-right)
    float sliderX = x + VU_METER_WIDTH + 10 + (width - VU_METER_WIDTH - 15 - SLIDER_WIDTH) / 2;
    float sliderY = currentY;
    float volumeDB = linearToDB(track->getVolume());
    drawVolumeSlider(rt, sliderX, sliderY, SLIDER_WIDTH, SLIDER_HEIGHT, volumeDB, false);

    // Add control for volume slider
    Control volumeControl;
    volumeControl.type = ControlType::VolumeSlider;
    volumeControl.trackIndex = trackIndex;
    volumeControl.x = sliderX;
    volumeControl.y = sliderY;
    volumeControl.w = SLIDER_WIDTH;
    volumeControl.h = SLIDER_HEIGHT;
    m_controls.push_back(volumeControl);

    currentY += SLIDER_HEIGHT + 20;

    // Volume label in dB
    wchar_t volLabel[32];
    if (volumeDB <= MIN_DB) {
        swprintf(volLabel, 32, L"-inf");
    } else {
        swprintf(volLabel, 32, L"%.1f dB", volumeDB);
    }
    drawText(volLabel, x + width / 2 - 20, currentY, DAWColors::TextSecondary);

    currentY += 30;

    // Pan knob
    float panKnobX = x + width / 2;
    float panKnobY = currentY + KNOB_RADIUS;
    // Pan value: -1 (left) to +1 (right), map to 0-1 for display
    float panNormalized = (track->getPan() + 1.0f) / 2.0f;
    drawRotaryKnob(rt, panKnobX, panKnobY, KNOB_RADIUS, panNormalized, false, L"Pan");

    // Add control for pan knob
    Control panControl;
    panControl.type = ControlType::PanKnob;
    panControl.trackIndex = trackIndex;
    panControl.x = panKnobX - KNOB_RADIUS;
    panControl.y = panKnobY - KNOB_RADIUS;
    panControl.w = KNOB_RADIUS * 2;
    panControl.h = KNOB_RADIUS * 2;
    m_controls.push_back(panControl);

    currentY += KNOB_RADIUS * 2 + 35;

    // Pan value text
    wchar_t panLabel[32];
    if (track->getPan() < -0.05f) {
        swprintf(panLabel, 32, L"L%.0f", -track->getPan() * 100);
    } else if (track->getPan() > 0.05f) {
        swprintf(panLabel, 32, L"R%.0f", track->getPan() * 100);
    } else {
        swprintf(panLabel, 32, L"C");
    }
    drawText(panLabel, x + width / 2 - 15, currentY, DAWColors::TextSecondary);

    currentY += 30;

    // EQ knobs section
    drawText(L"EQ", x + width / 2 - 10, currentY, DAWColors::TextSecondary);
    currentY += 25;

    // Get EQ values from track and normalize to 0-1 range (0.5 = 0 dB)
    float eqLow = (track->getEQLow() + 12.0f) / 24.0f;   // -12 to +12 dB -> 0 to 1
    float eqMid = (track->getEQMid() + 12.0f) / 24.0f;
    float eqHigh = (track->getEQHigh() + 12.0f) / 24.0f;

    // Low EQ knob
    float eq1X = x + width / 4;
    float eq1Y = currentY + KNOB_RADIUS * 0.7f;
    drawRotaryKnob(rt, eq1X, eq1Y, KNOB_RADIUS * 0.7f, eqLow, false, L"Low");

    Control eq1Control;
    eq1Control.type = ControlType::LowEQKnob;
    eq1Control.trackIndex = trackIndex;
    eq1Control.x = eq1X - KNOB_RADIUS * 0.7f;
    eq1Control.y = eq1Y - KNOB_RADIUS * 0.7f;
    eq1Control.w = KNOB_RADIUS * 1.4f;
    eq1Control.h = KNOB_RADIUS * 1.4f;
    m_controls.push_back(eq1Control);

    // Mid EQ knob (centered)
    float eq2X = x + width / 2;
    float eq2Y = currentY + KNOB_RADIUS * 0.7f;
    drawRotaryKnob(rt, eq2X, eq2Y, KNOB_RADIUS * 0.7f, eqMid, false, L"Mid");

    Control eq2Control;
    eq2Control.type = ControlType::MidEQKnob;
    eq2Control.trackIndex = trackIndex;
    eq2Control.x = eq2X - KNOB_RADIUS * 0.7f;
    eq2Control.y = eq2Y - KNOB_RADIUS * 0.7f;
    eq2Control.w = KNOB_RADIUS * 1.4f;
    eq2Control.h = KNOB_RADIUS * 1.4f;
    m_controls.push_back(eq2Control);

    // High EQ knob
    float eq3X = x + width * 3 / 4;
    float eq3Y = currentY + KNOB_RADIUS * 0.7f;
    drawRotaryKnob(rt, eq3X, eq3Y, KNOB_RADIUS * 0.7f, eqHigh, false, L"High");

    Control eq3Control;
    eq3Control.type = ControlType::HighEQKnob;
    eq3Control.trackIndex = trackIndex;
    eq3Control.x = eq3X - KNOB_RADIUS * 0.7f;
    eq3Control.y = eq3Y - KNOB_RADIUS * 0.7f;
    eq3Control.w = KNOB_RADIUS * 1.4f;
    eq3Control.h = KNOB_RADIUS * 1.4f;
    m_controls.push_back(eq3Control);

    currentY += KNOB_RADIUS * 1.4f + 35;

    // Mute and Solo buttons at bottom
    float buttonY = y + height - BUTTON_HEIGHT - 10;
    float muteX = x + 10;
    float soloX = x + width / 2 + 5;
    float buttonWidth = (width - 30) / 2;

    drawButton(rt, muteX, buttonY, buttonWidth, BUTTON_HEIGHT, L"M", track->isMuted(), false);
    drawButton(rt, soloX, buttonY, buttonWidth, BUTTON_HEIGHT, L"S", track->isSolo(), false);

    // Add controls for buttons
    Control muteControl;
    muteControl.type = ControlType::MuteButton;
    muteControl.trackIndex = trackIndex;
    muteControl.x = muteX;
    muteControl.y = buttonY;
    muteControl.w = buttonWidth;
    muteControl.h = BUTTON_HEIGHT;
    m_controls.push_back(muteControl);

    Control soloControl;
    soloControl.type = ControlType::SoloButton;
    soloControl.trackIndex = trackIndex;
    soloControl.x = soloX;
    soloControl.y = buttonY;
    soloControl.w = buttonWidth;
    soloControl.h = BUTTON_HEIGHT;
    m_controls.push_back(soloControl);
}

void MixerWindow::drawVUMeter(ID2D1RenderTarget* rt, float x, float y, float width, float height, float peakLevel) {
    // Draw VU meter background
    fillRect(x, y, width, height, DAWColors::Timeline);
    drawRect(x, y, width, height, DAWColors::GridLine, 1.0f);

    // Convert peak level to dB
    float peakDB = linearToDB(peakLevel);

    // Map dB to height (MIN_DB at bottom, 0 dB at top)
    float normalized = (peakDB - MIN_DB) / (MAX_DB - MIN_DB);
    normalized = std::max(0.0f, std::min(1.0f, normalized));

    float fillHeight = normalized * height;
    float fillY = y + height - fillHeight;

    // Draw meter with color gradient (green -> yellow -> red)
    float greenThreshold = (-6.0f - MIN_DB) / (MAX_DB - MIN_DB);  // -6 dB
    float yellowThreshold = (-3.0f - MIN_DB) / (MAX_DB - MIN_DB); // -3 dB

    // Draw segments
    if (normalized > 0) {
        float currentY = y + height;
        float segmentHeight = fillHeight;

        // Green section (below -6 dB)
        if (normalized <= greenThreshold) {
            fillRect(x, fillY, width, fillHeight, Color::fromRGB(50, 200, 50));
        } else {
            float greenHeight = greenThreshold * height;
            fillRect(x, y + height - greenHeight, width, greenHeight, Color::fromRGB(50, 200, 50));

            // Yellow section (-6 to -3 dB)
            if (normalized <= yellowThreshold) {
                float yellowHeight = (normalized - greenThreshold) * height;
                fillRect(x, fillY, width, yellowHeight, Color::fromRGB(200, 200, 50));
            } else {
                float yellowHeight = (yellowThreshold - greenThreshold) * height;
                fillRect(x, y + height - greenHeight - yellowHeight, width, yellowHeight, Color::fromRGB(200, 200, 50));

                // Red section (above -3 dB)
                float redHeight = (normalized - yellowThreshold) * height;
                fillRect(x, fillY, width, redHeight, Color::fromRGB(200, 50, 50));
            }
        }
    }

    // Draw 0 dB line
    float zeroDBY = y + height * (1.0f - (0.0f - MIN_DB) / (MAX_DB - MIN_DB));
    drawLine(x, zeroDBY, x + width, zeroDBY, DAWColors::TextPrimary, 1.0f);
}

void MixerWindow::drawVolumeSlider(ID2D1RenderTarget* rt, float x, float y, float width, float height, float volumeDB, bool hovered) {
    // Draw slider track
    fillRect(x, y, width, height, DAWColors::Timeline);
    drawRect(x, y, width, height, DAWColors::GridLine, 1.0f);

    // Map dB to slider position
    float normalized = (volumeDB - MIN_DB) / (MAX_DB - MIN_DB);
    normalized = std::max(0.0f, std::min(1.0f, normalized));

    // Draw filled portion
    float fillHeight = normalized * height;
    float fillY = y + height - fillHeight;
    fillRect(x, fillY, width, fillHeight, hovered ? DAWColors::WaveformPeak : DAWColors::Waveform);

    // Draw 0 dB line
    float zeroDBY = y + height * (1.0f - (0.0f - MIN_DB) / (MAX_DB - MIN_DB));
    drawLine(x - 2, zeroDBY, x + width + 2, zeroDBY, Color::fromRGB(150, 150, 150), 1.0f);

    // Draw slider thumb
    float thumbY = y + (1.0f - normalized) * height;
    float thumbHeight = 6.0f;
    fillRect(x - 2, thumbY - thumbHeight / 2, width + 4, thumbHeight,
             hovered ? DAWColors::TextPrimary : DAWColors::TextSecondary);
}

void MixerWindow::drawRotaryKnob(ID2D1RenderTarget* rt, float x, float y, float radius, float value, bool hovered, const wchar_t* label) {
    // Draw outer circle
    ID2D1EllipseGeometry* ellipse = nullptr;
    auto factory = Application::getInstance().getD2DFactory();
    factory->CreateEllipseGeometry(D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius), &ellipse);

    if (ellipse) {
        auto brush = getBrush();
        brush->SetColor(hovered ? DAWColors::ButtonHover.toD2D() : DAWColors::ButtonNormal.toD2D());
        rt->FillGeometry(ellipse, brush);

        brush->SetColor(DAWColors::GridLine.toD2D());
        rt->DrawGeometry(ellipse, brush, 2.0f);

        ellipse->Release();
    }

    // Draw value indicator (tick mark)
    // Value range: 0 to 1, mapped to angle from 225 degrees to -45 degrees (270 degree sweep)
    float startAngle = 225.0f * static_cast<float>(M_PI) / 180.0f;  // Bottom-left
    float endAngle = -45.0f * static_cast<float>(M_PI) / 180.0f;    // Bottom-right
    float angle = startAngle + value * (endAngle - startAngle);

    float tickLength = radius * 0.6f;
    float tickStartX = x + cos(angle) * (radius - tickLength);
    float tickStartY = y + sin(angle) * (radius - tickLength);
    float tickEndX = x + cos(angle) * (radius - 2);
    float tickEndY = y + sin(angle) * (radius - 2);

    drawLine(tickStartX, tickStartY, tickEndX, tickEndY,
             hovered ? DAWColors::TextPrimary : DAWColors::TextSecondary, 2.0f);

    // Draw label below
    if (label) {
        float textWidth = wcslen(label) * 7.0f;  // Approximate
        drawText(label, x - textWidth / 2, y + radius + 5, DAWColors::TextSecondary);
    }
}

void MixerWindow::drawButton(ID2D1RenderTarget* rt, float x, float y, float width, float height, const wchar_t* text, bool active, bool hovered) {
    Color bgColor;
    if (active) {
        bgColor = (text[0] == L'M') ? Color::fromRGB(200, 50, 50) : Color::fromRGB(200, 180, 50);
    } else if (hovered) {
        bgColor = DAWColors::ButtonHover;
    } else {
        bgColor = DAWColors::ButtonNormal;
    }

    fillRect(x, y, width, height, bgColor);
    drawRect(x, y, width, height, DAWColors::GridLine, 1.0f);

    // Center text
    float textWidth = wcslen(text) * 8.0f;  // Approximate
    drawText(text, x + (width - textWidth) / 2, y + (height - 16) / 2, DAWColors::TextPrimary);
}

MixerWindow::Control* MixerWindow::getControlAtPosition(int x, int y) {
    for (auto& control : m_controls) {
        if (x >= control.x && x <= control.x + control.w &&
            y >= control.y && y <= control.y + control.h) {
            return &control;
        }
    }
    return nullptr;
}

float MixerWindow::getValueFromSliderY(int y, float sliderY, float sliderHeight) {
    float relativeY = y - sliderY;
    float normalized = 1.0f - (relativeY / sliderHeight);
    return std::max(0.0f, std::min(1.0f, normalized));
}

float MixerWindow::getValueFromKnobAngle(int mouseX, int mouseY, float knobCenterX, float knobCenterY) {
    float dx = mouseX - knobCenterX;
    float dy = mouseY - knobCenterY;
    float angle = atan2(dy, dx);

    // Map angle to 0-1 range
    // Start angle: 225 degrees (bottom-left) = 0.0
    // End angle: -45 degrees (bottom-right) = 1.0
    float startAngle = 225.0f * static_cast<float>(M_PI) / 180.0f;
    float endAngle = -45.0f * static_cast<float>(M_PI) / 180.0f;

    // Normalize angle to 0-2π range
    if (angle < 0) angle += 2.0f * static_cast<float>(M_PI);

    // Convert to value (0-1)
    float totalSweep = startAngle - endAngle;
    if (totalSweep < 0) totalSweep += 2.0f * static_cast<float>(M_PI);

    float angleFromStart = startAngle - angle;
    if (angleFromStart < 0) angleFromStart += 2.0f * static_cast<float>(M_PI);

    float value = angleFromStart / totalSweep;
    return std::max(0.0f, std::min(1.0f, value));
}

void MixerWindow::onResize(int width, int height) {
    invalidate();
}

void MixerWindow::onMouseDown(int x, int y, int button) {
    if (button != 0 || !m_tracks) return;  // Left button only

    Control* control = getControlAtPosition(x, y);
    if (!control || control->trackIndex >= static_cast<int>(m_tracks->size())) {
        return;
    }

    auto track = (*m_tracks)[control->trackIndex];
    m_draggedControl = control;

    switch (control->type) {
        case ControlType::MuteButton:
            track->setMuted(!track->isMuted());
            if (m_changeCallback) m_changeCallback();
            invalidate();
            break;

        case ControlType::SoloButton:
            track->setSolo(!track->isSolo());
            if (m_changeCallback) m_changeCallback();
            invalidate();
            break;

        case ControlType::VolumeSlider: {
            float newVolumeDB = getDBFromSliderY(y, control->y, control->h);
            float newVolume = dbToLinear(newVolumeDB);
            track->setVolume(newVolume);
            if (m_changeCallback) m_changeCallback();
            invalidate();
            break;
        }

        case ControlType::PanKnob: {
            float knobCenterX = control->x + control->w / 2;
            float knobCenterY = control->y + control->h / 2;
            float value = getValueFromKnobAngle(x, y, knobCenterX, knobCenterY);
            float pan = value * 2.0f - 1.0f;  // Convert 0-1 to -1 to +1
            track->setPan(pan);
            if (m_changeCallback) m_changeCallback();
            invalidate();
            break;
        }

        case ControlType::LowEQKnob:
        case ControlType::MidEQKnob:
        case ControlType::HighEQKnob: {
            float knobCenterX = control->x + control->w / 2;
            float knobCenterY = control->y + control->h / 2;
            float value = getValueFromKnobAngle(x, y, knobCenterX, knobCenterY);
            float eqGain = value * 24.0f - 12.0f;  // Convert 0-1 to -12 to +12 dB

            if (control->type == ControlType::LowEQKnob) {
                track->setEQLow(eqGain);
            } else if (control->type == ControlType::MidEQKnob) {
                track->setEQMid(eqGain);
            } else if (control->type == ControlType::HighEQKnob) {
                track->setEQHigh(eqGain);
            }

            if (m_changeCallback) m_changeCallback();
            invalidate();
            break;
        }

        default:
            break;
    }
}

void MixerWindow::onMouseUp(int x, int y, int button) {
    if (button != 0) return;

    m_draggedControl = nullptr;
    invalidate();
}

void MixerWindow::onMouseMove(int x, int y) {
    if (!m_draggedControl || !m_tracks) return;

    if (m_draggedControl->trackIndex >= static_cast<int>(m_tracks->size())) {
        return;
    }

    auto track = (*m_tracks)[m_draggedControl->trackIndex];

    switch (m_draggedControl->type) {
        case ControlType::VolumeSlider: {
            float newVolumeDB = getDBFromSliderY(y, m_draggedControl->y, m_draggedControl->h);
            float newVolume = dbToLinear(newVolumeDB);
            track->setVolume(newVolume);
            if (m_changeCallback) m_changeCallback();
            invalidate();
            break;
        }

        case ControlType::PanKnob: {
            float knobCenterX = m_draggedControl->x + m_draggedControl->w / 2;
            float knobCenterY = m_draggedControl->y + m_draggedControl->h / 2;
            float value = getValueFromKnobAngle(x, y, knobCenterX, knobCenterY);
            float pan = value * 2.0f - 1.0f;  // Convert 0-1 to -1 to +1
            track->setPan(pan);
            if (m_changeCallback) m_changeCallback();
            invalidate();
            break;
        }

        case ControlType::LowEQKnob:
        case ControlType::MidEQKnob:
        case ControlType::HighEQKnob: {
            float knobCenterX = m_draggedControl->x + m_draggedControl->w / 2;
            float knobCenterY = m_draggedControl->y + m_draggedControl->h / 2;
            float value = getValueFromKnobAngle(x, y, knobCenterX, knobCenterY);
            float eqGain = value * 24.0f - 12.0f;  // Convert 0-1 to -12 to +12 dB

            if (m_draggedControl->type == ControlType::LowEQKnob) {
                track->setEQLow(eqGain);
            } else if (m_draggedControl->type == ControlType::MidEQKnob) {
                track->setEQMid(eqGain);
            } else if (m_draggedControl->type == ControlType::HighEQKnob) {
                track->setEQHigh(eqGain);
            }

            if (m_changeCallback) m_changeCallback();
            invalidate();
            break;
        }

        default:
            break;
    }
}
