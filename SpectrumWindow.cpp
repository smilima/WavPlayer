#include "SpectrumWindow.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SpectrumWindow::SpectrumWindow() {
    m_fftBuffer.resize(FFT_SIZE);
    m_magnitudes.resize(FFT_SIZE / 2);
    m_bandValues.resize(NUM_BANDS, 0.0f);
    m_bandPeaks.resize(NUM_BANDS, 0.0f);
    m_bandFreqs.resize(NUM_BANDS);

    // Use standard ISO graphic EQ center frequencies
    for (int i = 0; i < NUM_BANDS; i++) {
        m_bandFreqs[i] = BAND_FREQUENCIES[i];
    }

    // Initialize EQ gains to 0 dB (flat response)
    m_eqGains.fill(0.0f);
}

SpectrumWindow::~SpectrumWindow() {
}

void SpectrumWindow::clear() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    std::fill(m_bandValues.begin(), m_bandValues.end(), 0.0f);
    std::fill(m_bandPeaks.begin(), m_bandPeaks.end(), 0.0f);
    invalidate();
}

void SpectrumWindow::updateSpectrum(const float* samples, size_t sampleCount, int sampleRate) {
    if (!samples || sampleCount == 0) {
        return;
    }

    m_sampleRate = sampleRate;

    // Copy samples to FFT buffer (taking first FFT_SIZE samples)
    std::vector<float> audioSamples(FFT_SIZE, 0.0f);

    // Assuming stereo interleaved: sampleCount is total samples, frames = sampleCount / 2
    size_t frameCount = sampleCount / 2;
    size_t copyFrames = std::min(frameCount, static_cast<size_t>(FFT_SIZE));

    // Convert stereo to mono (averaging left and right channels)
    for (size_t i = 0; i < copyFrames; i++) {
        audioSamples[i] = (samples[i * 2] + samples[i * 2 + 1]) * 0.5f;
    }

    performFFT(audioSamples);
    invalidate();
}

void SpectrumWindow::performFFT(const std::vector<float>& samples) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    // Apply Hann window and copy to FFT buffer
    for (int i = 0; i < FFT_SIZE; i++) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (FFT_SIZE - 1)));
        m_fftBuffer[i] = std::complex<float>(samples[i] * window, 0.0f);
    }

    // Perform FFT
    fft(m_fftBuffer, false);

    // Calculate magnitudes
    for (int i = 0; i < FFT_SIZE / 2; i++) {
        float real = m_fftBuffer[i].real();
        float imag = m_fftBuffer[i].imag();
        m_magnitudes[i] = std::sqrt(real * real + imag * imag);
    }

    // Calculate band values
    calculateBands();
}

void SpectrumWindow::fft(std::vector<std::complex<float>>& buffer, bool inverse) {
    int n = static_cast<int>(buffer.size());

    // Bit-reversal permutation
    int j = 0;
    for (int i = 0; i < n - 1; i++) {
        if (i < j) {
            std::swap(buffer[i], buffer[j]);
        }
        int k = n / 2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }

    // FFT computation
    float sign = inverse ? 1.0f : -1.0f;
    for (int len = 2; len <= n; len *= 2) {
        float angle = sign * 2.0f * M_PI / len;
        std::complex<float> wlen(std::cos(angle), std::sin(angle));

        for (int i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (int j = 0; j < len / 2; j++) {
                std::complex<float> u = buffer[i + j];
                std::complex<float> v = buffer[i + j + len / 2] * w;
                buffer[i + j] = u + v;
                buffer[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }

    if (inverse) {
        for (int i = 0; i < n; i++) {
            buffer[i] /= static_cast<float>(n);
        }
    }
}

void SpectrumWindow::calculateBands() {
    float freqPerBin = static_cast<float>(m_sampleRate) / FFT_SIZE;

    for (int band = 0; band < NUM_BANDS; band++) {
        // Calculate frequency range for this band
        float centerFreq = m_bandFreqs[band];
        float freqStart, freqEnd;

        if (band == 0) {
            freqStart = MIN_FREQ;
            freqEnd = std::sqrt(m_bandFreqs[0] * m_bandFreqs[1]);
        } else if (band == NUM_BANDS - 1) {
            freqStart = std::sqrt(m_bandFreqs[NUM_BANDS - 2] * m_bandFreqs[NUM_BANDS - 1]);
            freqEnd = MAX_FREQ;
        } else {
            freqStart = std::sqrt(m_bandFreqs[band - 1] * m_bandFreqs[band]);
            freqEnd = std::sqrt(m_bandFreqs[band] * m_bandFreqs[band + 1]);
        }

        // Find corresponding FFT bins
        int binStart = static_cast<int>(freqStart / freqPerBin);
        int binEnd = static_cast<int>(freqEnd / freqPerBin);
        binStart = std::max(0, std::min(binStart, FFT_SIZE / 2 - 1));
        binEnd = std::max(0, std::min(binEnd, FFT_SIZE / 2 - 1));

        // Average magnitude in this frequency range
        float sum = 0.0f;
        int count = 0;
        for (int bin = binStart; bin <= binEnd; bin++) {
            sum += m_magnitudes[bin];
            count++;
        }

        float avgMagnitude = (count > 0) ? (sum / count) : 0.0f;

        // Normalize magnitude relative to FFT size
        // For a full-scale signal, FFT magnitude is approximately FFT_SIZE/2 after windowing
        float normalizedMag = avgMagnitude / (FFT_SIZE / 4.0f);

        // Convert to dB scale (with better range for typical audio)
        float dB = 20.0f * std::log10(normalizedMag + 1e-10f);

        // Map dB range to 0-1 (adjusted for better visualization)
        // -80dB (quiet) -> 0, -20dB (loud) -> 1
        float normalized = (dB + 80.0f) / 60.0f;
        normalized = std::max(0.0f, std::min(1.0f, normalized));

        // Apply smoothing
        m_bandValues[band] = m_bandValues[band] * SMOOTHING + normalized * (1.0f - SMOOTHING);

        // Update peaks with decay
        m_bandPeaks[band] *= PEAK_DECAY;
        if (m_bandValues[band] > m_bandPeaks[band]) {
            m_bandPeaks[band] = m_bandValues[band];
        }
    }
}

void SpectrumWindow::BiquadFilter::calculatePeakingEQ(float centerFreq, float gainDB, float Q, float sampleRate) {
    float A = std::pow(10.0f, gainDB / 40.0f);  // sqrt of linear gain
    float omega = 2.0f * M_PI * centerFreq / sampleRate;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * Q);

    // RBJ Audio EQ Cookbook - Peaking EQ
    float a0 = 1.0f + alpha / A;
    b0 = (1.0f + alpha * A) / a0;
    b1 = (-2.0f * cosOmega) / a0;
    b2 = (1.0f - alpha * A) / a0;
    a1 = (-2.0f * cosOmega) / a0;
    a2 = (1.0f - alpha / A) / a0;
}

float SpectrumWindow::BiquadFilter::process(float input, float& z1, float& z2) {
    // Direct Form II Transposed
    float output = input * b0 + z1;
    z1 = input * b1 + z2 - a1 * output;
    z2 = input * b2 - a2 * output;
    return output;
}

void SpectrumWindow::updateFilters() {
    for (int i = 0; i < NUM_BANDS; i++) {
        m_filters[i].calculatePeakingEQ(BAND_FREQUENCIES[i], m_eqGains[i], Q_FACTOR, m_sampleRate);
    }
    m_filtersInitialized = true;
}

void SpectrumWindow::applyEQ(float* samples, size_t frameCount, int sampleRate) {
    if (m_sampleRate != sampleRate) {
        m_sampleRate = sampleRate;
        m_filtersInitialized = false;
    }

    if (!m_filtersInitialized) {
        updateFilters();
    }

    // Process each frame (stereo interleaved: L, R, L, R, ...)
    for (size_t frame = 0; frame < frameCount; frame++) {
        float left = samples[frame * 2];
        float right = samples[frame * 2 + 1];

        // Apply all EQ bands in series
        for (int band = 0; band < NUM_BANDS; band++) {
            left = m_filters[band].process(left, m_filters[band].z1_L, m_filters[band].z2_L);
            right = m_filters[band].process(right, m_filters[band].z1_R, m_filters[band].z2_R);
        }

        samples[frame * 2] = left;
        samples[frame * 2 + 1] = right;
    }
}

int SpectrumWindow::getSliderAtPosition(int x, int y) {
    float margin = 20.0f;
    float topMargin = 40.0f;
    float bottomMargin = 60.0f;
    float width = static_cast<float>(getWidth());
    float height = static_cast<float>(getHeight());
    float usableWidth = width - 2 * margin;
    float usableHeight = height - topMargin - bottomMargin;

    float barWidth = usableWidth / NUM_BANDS;

    // Check if within vertical bounds
    if (y < topMargin || y > height - bottomMargin) {
        return -1;
    }

    // Find which band the x position corresponds to
    float relativeX = x - margin;
    if (relativeX < 0 || relativeX > usableWidth) {
        return -1;
    }

    int band = static_cast<int>(relativeX / barWidth);
    if (band >= 0 && band < NUM_BANDS) {
        return band;
    }

    return -1;
}

float SpectrumWindow::getGainFromY(int y) {
    float topMargin = 40.0f;
    float bottomMargin = 60.0f;
    float height = static_cast<float>(getHeight());
    float usableHeight = height - topMargin - bottomMargin;

    // Map y position to gain range (-12 to +12 dB)
    float relativeY = y - topMargin;
    float normalizedY = std::max(0.0f, std::min(1.0f, relativeY / usableHeight));

    // Invert: top = +12 dB, bottom = -12 dB
    float gain = 12.0f - (normalizedY * 24.0f);

    return std::max(-12.0f, std::min(12.0f, gain));
}

void SpectrumWindow::onRender(ID2D1RenderTarget* rt) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    // Clear background
    fillRect(0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight()),
             DAWColors::Background);

    // Draw title
    drawText(L"12-Band Graphic Equalizer", 10, 10, DAWColors::TextPrimary);

    // Calculate dimensions
    float margin = 20.0f;
    float topMargin = 40.0f;
    float bottomMargin = 60.0f;
    float width = static_cast<float>(getWidth());
    float height = static_cast<float>(getHeight());
    float usableWidth = width - 2 * margin;
    float usableHeight = height - topMargin - bottomMargin;

    float barWidth = usableWidth / NUM_BANDS;
    float barSpacing = barWidth * 0.1f;
    float barActualWidth = barWidth - barSpacing;

    // Draw 0dB reference line (center)
    float zeroDbY = topMargin + usableHeight / 2;
    drawLine(margin, zeroDbY, width - margin, zeroDbY, Color(0.5f, 0.5f, 0.5f), 2.0f);

    // Draw grid lines for EQ (+12, +6, 0, -6, -12 dB)
    for (int i = 0; i <= 4; i++) {
        float y = topMargin + (usableHeight * i) / 4;
        drawLine(margin, y, width - margin, y, DAWColors::GridLine, 1.0f);

        // EQ gain labels
        wchar_t dbLabel[16];
        int dB = 12 - i * 6;  // +12, +6, 0, -6, -12 dB
        swprintf(dbLabel, 16, L"%+d dB", dB);
        drawText(dbLabel, 5, y - 8, DAWColors::TextSecondary);
    }

    // Draw frequency labels (every other band for 12-band)
    for (int i = 0; i < NUM_BANDS; i += 2) {
        float x = margin + i * barWidth;
        float freq = BAND_FREQUENCIES[i];

        wchar_t label[32];
        if (freq < 1000.0f) {
            swprintf(label, 32, L"%.0f", freq);
        } else {
            swprintf(label, 32, L"%.1fk", freq / 1000.0f);
        }

        drawText(label, x, height - bottomMargin + 10, DAWColors::TextSecondary);
    }

    // Draw spectrum bars (dimmed, in background)
    for (int i = 0; i < NUM_BANDS; i++) {
        float x = margin + i * barWidth + barSpacing / 2;
        float barHeight = m_bandValues[i] * usableHeight;
        float y = height - bottomMargin - barHeight;

        // Dimmed gradient color
        float hue = (1.0f - m_bandValues[i]) * 0.6f;
        Color barColor;
        if (hue > 0.33f) {
            barColor = Color(0.0f, (0.6f - hue) * 3.0f * 0.3f, 1.0f * 0.3f);
        } else if (hue > 0.16f) {
            barColor = Color((0.33f - hue) * 6.0f * 0.3f, 1.0f * 0.3f, (hue - 0.16f) * 6.0f * 0.3f);
        } else {
            barColor = Color(1.0f * 0.3f, hue * 6.0f * 0.3f, 0.0f);
        }

        fillRect(x, y, barActualWidth, barHeight, barColor);
    }

    // Draw EQ sliders on top
    float sliderWidth = barActualWidth * 0.4f;
    for (int i = 0; i < NUM_BANDS; i++) {
        float x = margin + i * barWidth + barWidth / 2;

        // Calculate slider position from gain (-12 to +12 dB)
        float normalizedGain = (12.0f - m_eqGains[i]) / 24.0f;  // 0 = +12dB, 1 = -12dB
        float sliderY = topMargin + normalizedGain * usableHeight;

        // Draw vertical slider track
        float trackX = x - sliderWidth / 2;
        drawLine(x, topMargin, x, height - bottomMargin,
                 DAWColors::GridLine, 2.0f);

        // Draw slider knob
        bool isHovered = (m_draggedSlider == i && m_isDragging);
        Color knobColor = isHovered ? DAWColors::WaveformPeak : DAWColors::TextPrimary;

        float knobSize = 8.0f;
        fillRect(trackX, sliderY - knobSize / 2, sliderWidth, knobSize, knobColor);

        // Draw gain value if being dragged
        if (isHovered) {
            wchar_t gainLabel[16];
            swprintf(gainLabel, 16, L"%+.1f", m_eqGains[i]);
            drawText(gainLabel, trackX - 20, sliderY - 8, DAWColors::TextPrimary);
        }
    }
}

void SpectrumWindow::onResize(int width, int height) {
    invalidate();
}

void SpectrumWindow::onMouseDown(int x, int y, int button) {
    if (button != 0) return;  // Left button only

    int slider = getSliderAtPosition(x, y);
    if (slider >= 0) {
        m_draggedSlider = slider;
        m_isDragging = true;

        // Update gain immediately
        float newGain = getGainFromY(y);
        if (std::abs(m_eqGains[slider] - newGain) > 0.1f) {
            m_eqGains[slider] = newGain;
            m_filtersInitialized = false;  // Force filter recalculation
            invalidate();
        }
    }
}

void SpectrumWindow::onMouseUp(int x, int y, int button) {
    if (button != 0) return;

    if (m_isDragging) {
        m_isDragging = false;
        m_draggedSlider = -1;
        invalidate();
    }
}

void SpectrumWindow::onMouseMove(int x, int y) {
    if (m_isDragging && m_draggedSlider >= 0) {
        float newGain = getGainFromY(y);
        if (std::abs(m_eqGains[m_draggedSlider] - newGain) > 0.1f) {
            m_eqGains[m_draggedSlider] = newGain;
            m_filtersInitialized = false;  // Force filter recalculation
            invalidate();
        }
    }
}
