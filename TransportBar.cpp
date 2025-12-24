#include "TransportBar.h"
#include "Application.h"
#include <sstream>
#include <iomanip>

void TransportBar::setPosition(double seconds) {
    if (m_position != seconds) {
        m_position = seconds;
        invalidate();
    }
}

void TransportBar::setDuration(double seconds) {
    if (m_duration != seconds) {
        m_duration = seconds;
        invalidate();
    }
}

void TransportBar::onResize(int width, int height) {
    // Only re-layout if size actually changed
    if (m_lastWidth != width || m_lastHeight != height) {
        m_lastWidth = width;
        m_lastHeight = height;
        m_buttonsInitialized = false;
    }
}

void TransportBar::initializeGeometries() {
    if (m_playGeometry) return; // Already initialized

    auto factory = Application::getInstance().getD2DFactory();

    // Create Play geometry (triangle pointing right)
    factory->CreatePathGeometry(&m_playGeometry);
    if (m_playGeometry) {
        ID2D1GeometrySink* sink = nullptr;
        m_playGeometry->Open(&sink);
        if (sink) {
            // Unit size triangle, will be scaled during rendering
            float cx = 0.0f, cy = 0.0f, size = 1.0f;
            sink->BeginFigure(D2D1::Point2F(-size * 0.4f, -size * 0.6f), D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLine(D2D1::Point2F(size * 0.6f, 0.0f));
            sink->AddLine(D2D1::Point2F(-size * 0.4f, size * 0.6f));
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            sink->Release();
        }
    }

    // Create Rewind geometry (two triangles pointing left)
    factory->CreatePathGeometry(&m_rewindGeometry);
    if (m_rewindGeometry) {
        ID2D1GeometrySink* sink = nullptr;
        m_rewindGeometry->Open(&sink);
        if (sink) {
            float triSize = 0.5f;
            // First triangle
            sink->BeginFigure(D2D1::Point2F(0.0f, -triSize), D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLine(D2D1::Point2F(-triSize, 0.0f));
            sink->AddLine(D2D1::Point2F(0.0f, triSize));
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            // Second triangle
            sink->BeginFigure(D2D1::Point2F(triSize, -triSize), D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLine(D2D1::Point2F(0.0f, 0.0f));
            sink->AddLine(D2D1::Point2F(triSize, triSize));
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            sink->Release();
        }
    }

    // Create Fast Forward geometry (two triangles pointing right)
    factory->CreatePathGeometry(&m_fastForwardGeometry);
    if (m_fastForwardGeometry) {
        ID2D1GeometrySink* sink = nullptr;
        m_fastForwardGeometry->Open(&sink);
        if (sink) {
            float triSize = 0.5f;
            // First triangle
            sink->BeginFigure(D2D1::Point2F(-triSize, -triSize), D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLine(D2D1::Point2F(0.0f, 0.0f));
            sink->AddLine(D2D1::Point2F(-triSize, triSize));
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            // Second triangle
            sink->BeginFigure(D2D1::Point2F(0.0f, -triSize), D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLine(D2D1::Point2F(triSize, 0.0f));
            sink->AddLine(D2D1::Point2F(0.0f, triSize));
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            sink->Release();
        }
    }
}

void TransportBar::releaseGeometries() {
    if (m_playGeometry) {
        m_playGeometry->Release();
        m_playGeometry = nullptr;
    }
    if (m_rewindGeometry) {
        m_rewindGeometry->Release();
        m_rewindGeometry = nullptr;
    }
    if (m_fastForwardGeometry) {
        m_fastForwardGeometry->Release();
        m_fastForwardGeometry = nullptr;
    }
}

void TransportBar::layoutButtons() {
    m_buttons.clear();

    float btnSize = 36.0f;
    float spacing = 8.0f;
    float startX = 20.0f;
    float centerY = static_cast<float>(getHeight()) / 2.0f - btnSize / 2.0f;

    // Follow Playhead
    m_buttons.push_back({startX, centerY, btnSize, btnSize, Button::FollowPlayhead});
    startX += btnSize + spacing;

    // Rewind
    m_buttons.push_back({startX, centerY, btnSize, btnSize, Button::Rewind});
    startX += btnSize + spacing;

    // Stop
    m_buttons.push_back({startX, centerY, btnSize, btnSize, Button::Stop});
    startX += btnSize + spacing;

    // Play/Pause - only show Pause if playing AND audio is loaded
    Button::Type playType = (m_isPlaying && m_hasAudioLoaded) ? Button::Pause : Button::Play;
    m_buttons.push_back({startX, centerY, btnSize, btnSize, playType});
    startX += btnSize + spacing;

    // Fast Forward
    m_buttons.push_back({startX, centerY, btnSize, btnSize, Button::FastForward});
    startX += btnSize + spacing;

    // Record
    m_buttons.push_back({startX, centerY, btnSize, btnSize, Button::Record});

    m_buttonsInitialized = true;
}

void TransportBar::onRender(ID2D1RenderTarget* rt) {
    // Initialize geometries on first render
    if (!m_playGeometry) {
        initializeGeometries();
    }

    // Background
    fillRect(0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight()),
             DAWColors::Transport);

    // Top border
    drawLine(0, 0, static_cast<float>(getWidth()), 0, DAWColors::GridLine, 1.0f);

    if (!m_buttonsInitialized) {
        layoutButtons();
    }

    // Draw transport buttons
    for (const auto& btn : m_buttons) {
        drawButton(rt, btn);
    }

    // Time display - use cached strings when values haven't changed
    float timeX = 250.0f;
    float timeY = static_cast<float>(getHeight()) / 2.0f - 10.0f;

    // Cache position string
    if (m_cachedPosition != m_position) {
        m_cachedPosition = m_position;
        m_cachedPositionStr = formatTime(m_position);
    }
    drawText(m_cachedPositionStr, timeX, timeY, DAWColors::TextPrimary, 120, 20);

    // Separator
    drawText(L"/", timeX + 80, timeY, DAWColors::TextSecondary, 20, 20);

    // Cache duration string
    if (m_cachedDuration != m_duration) {
        m_cachedDuration = m_duration;
        m_cachedDurationStr = formatTime(m_duration);
    }
    drawText(m_cachedDurationStr, timeX + 100, timeY, DAWColors::TextSecondary, 120, 20);

    // Cache BPM string
    if (m_cachedBpm != m_bpm) {
        m_cachedBpm = m_bpm;
        std::wostringstream bpmStr;
        bpmStr << std::fixed << std::setprecision(1) << m_bpm << L" BPM";
        m_cachedBpmStr = bpmStr.str();
    }
    float bpmX = static_cast<float>(getWidth()) - 150.0f;
    drawText(m_cachedBpmStr, bpmX, timeY, DAWColors::TextSecondary, 100, 20);
}

void TransportBar::drawButton(ID2D1RenderTarget* rt, const Button& btn) {
    // Follow Playhead button is highlighted when active
    bool isActive = (btn.type == Button::FollowPlayhead && m_isFollowingPlayhead);
    Color bgColor = isActive ? DAWColors::ButtonPressed :
                    btn.pressed ? DAWColors::ButtonPressed :
                    btn.hovered ? DAWColors::ButtonHover : DAWColors::ButtonNormal;

    // Button background with rounded corners
    fillRect(btn.x, btn.y, btn.w, btn.h, bgColor);
    drawRect(btn.x, btn.y, btn.w, btn.h, DAWColors::GridLine, 1.0f);

    float cx = btn.x + btn.w / 2.0f;
    float cy = btn.y + btn.h / 2.0f;
    float iconSize = 12.0f;

    switch (btn.type) {
        case Button::FollowPlayhead:
            drawFollowPlayheadIcon(rt, cx, cy, iconSize);
            break;
        case Button::Play:
            drawPlayIcon(rt, cx, cy, iconSize);
            break;
        case Button::Pause:
            drawPauseIcon(rt, cx, cy, iconSize);
            break;
        case Button::Stop:
            drawStopIcon(rt, cx, cy, iconSize);
            break;
        case Button::Rewind:
            drawRewindIcon(rt, cx, cy, iconSize);
            break;
        case Button::FastForward:
            drawFastForwardIcon(rt, cx, cy, iconSize);
            break;
        case Button::Record:
            drawRecordIcon(rt, cx, cy, iconSize);
            break;
    }
}

void TransportBar::drawFollowPlayheadIcon(ID2D1RenderTarget* rt, float cx, float cy, float size) {
    // Draw a simple "F" character
    getBrush()->SetColor(DAWColors::TextPrimary.toD2D());

    // Draw text using Direct2D text rendering
    auto writeFactory = Application::getInstance().getDWriteFactory();
    IDWriteTextFormat* textFormat = nullptr;

    // Create text format for the "F" icon
    writeFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        size * 1.5f,  // Font size scaled with icon size
        L"en-us",
        &textFormat
    );

    if (textFormat) {
        textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Draw "F" centered at the button position
        D2D1_RECT_F textRect = D2D1::RectF(
            cx - size, cy - size,
            cx + size, cy + size
        );

        rt->DrawText(L"F", 1, textFormat, textRect, getBrush());
        textFormat->Release();
    }
}

void TransportBar::drawPlayIcon(ID2D1RenderTarget* rt, float cx, float cy, float size) {
    if (!m_playGeometry) return;

    // Use cached geometry with transformation
    D2D1::Matrix3x2F transform = D2D1::Matrix3x2F::Scale(size, size) *
                                  D2D1::Matrix3x2F::Translation(cx, cy);
    rt->SetTransform(transform);

    getBrush()->SetColor(DAWColors::TextPrimary.toD2D());
    rt->FillGeometry(m_playGeometry, getBrush());

    rt->SetTransform(D2D1::Matrix3x2F::Identity());
}

void TransportBar::drawPauseIcon(ID2D1RenderTarget* rt, float cx, float cy, float size) {
    float barWidth = size * 0.25f;
    float barHeight = size * 1.0f;
    float gap = size * 0.2f;
    
    getBrush()->SetColor(DAWColors::TextPrimary.toD2D());
    rt->FillRectangle(D2D1::RectF(cx - gap - barWidth, cy - barHeight / 2,
                                   cx - gap, cy + barHeight / 2), getBrush());
    rt->FillRectangle(D2D1::RectF(cx + gap, cy - barHeight / 2,
                                   cx + gap + barWidth, cy + barHeight / 2), getBrush());
}

void TransportBar::drawStopIcon(ID2D1RenderTarget* rt, float cx, float cy, float size) {
    float halfSize = size * 0.5f;
    getBrush()->SetColor(DAWColors::TextPrimary.toD2D());
    rt->FillRectangle(D2D1::RectF(cx - halfSize, cy - halfSize,
                                   cx + halfSize, cy + halfSize), getBrush());
}

void TransportBar::drawRewindIcon(ID2D1RenderTarget* rt, float cx, float cy, float size) {
    if (!m_rewindGeometry) return;

    // Use cached geometry with transformation
    D2D1::Matrix3x2F transform = D2D1::Matrix3x2F::Scale(size, size) *
                                  D2D1::Matrix3x2F::Translation(cx, cy);
    rt->SetTransform(transform);

    getBrush()->SetColor(DAWColors::TextPrimary.toD2D());
    rt->FillGeometry(m_rewindGeometry, getBrush());

    rt->SetTransform(D2D1::Matrix3x2F::Identity());
}

void TransportBar::drawFastForwardIcon(ID2D1RenderTarget* rt, float cx, float cy, float size) {
    if (!m_fastForwardGeometry) return;

    // Use cached geometry with transformation
    D2D1::Matrix3x2F transform = D2D1::Matrix3x2F::Scale(size, size) *
                                  D2D1::Matrix3x2F::Translation(cx, cy);
    rt->SetTransform(transform);

    getBrush()->SetColor(DAWColors::TextPrimary.toD2D());
    rt->FillGeometry(m_fastForwardGeometry, getBrush());

    rt->SetTransform(D2D1::Matrix3x2F::Identity());
}

void TransportBar::drawRecordIcon(ID2D1RenderTarget* rt, float cx, float cy, float size) {
    // Red circle for record
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(cx, cy), size * 0.6f, size * 0.6f);
    
    if (m_isRecording) {
        // Bright red when recording
        getBrush()->SetColor(D2D1::ColorF(0.95f, 0.2f, 0.2f));
    } else {
        // Darker red when not recording
        getBrush()->SetColor(D2D1::ColorF(0.7f, 0.2f, 0.2f));
    }
    
    rt->FillEllipse(ellipse, getBrush());
}

void TransportBar::onMouseDown(int x, int y, int button) {
    if (button != 0) return;
    
    for (auto& btn : m_buttons) {
        if (x >= btn.x && x <= btn.x + btn.w &&
            y >= btn.y && y <= btn.y + btn.h) {
            btn.pressed = true;
            invalidate();
            break;
        }
    }
}

void TransportBar::onMouseUp(int x, int y, int button) {
    if (button != 0) return;
    
    for (auto& btn : m_buttons) {
        if (btn.pressed) {
            btn.pressed = false;
            
            // Check if still over button (click completed)
            if (x >= btn.x && x <= btn.x + btn.w &&
                y >= btn.y && y <= btn.y + btn.h) {
                   switch (btn.type) {
                       case Button::FollowPlayhead:
                           if (m_onFollowPlayhead) m_onFollowPlayhead();
                           break;
                       case Button::Play:
                           // Only play if audio is loaded
                           if (m_hasAudioLoaded && m_onPlay) m_onPlay();
                           break;
                       case Button::Pause:
                           if (m_onPause) m_onPause();
                           break;
                       case Button::Stop:
                           if (m_onStop) m_onStop();
                           break;
                       case Button::Rewind:
                           if (m_onRewind) m_onRewind();
                           break;
                       case Button::FastForward:
                           if (m_onFastForward) m_onFastForward();
                           break;
                       case Button::Record:
                           if (m_onRecord) m_onRecord();
                           break;
                   }
               }
           }
       }
       
       layoutButtons();  // Refresh play/pause state
       invalidate();
   }

void TransportBar::onMouseMove(int x, int y) {
    bool needsRedraw = false;
    
    for (auto& btn : m_buttons) {
        bool wasHovered = btn.hovered;
        btn.hovered = (x >= btn.x && x <= btn.x + btn.w &&
                       y >= btn.y && y <= btn.y + btn.h);
        if (wasHovered != btn.hovered) {
            needsRedraw = true;
        }
    }
    
    if (needsRedraw) {
        invalidate();
    }
}

std::wstring TransportBar::formatTime(double seconds) {
    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    int ms = static_cast<int>((seconds - static_cast<int>(seconds)) * 1000);
    
    std::wostringstream ss;
    ss << std::setfill(L'0') << std::setw(2) << mins << L":"
       << std::setw(2) << secs << L"."
       << std::setw(3) << ms;
    return ss.str();
}
