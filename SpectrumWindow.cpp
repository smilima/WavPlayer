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

    // Calculate logarithmically-spaced band center frequencies
    float logMin = std::log10(MIN_FREQ);
    float logMax = std::log10(MAX_FREQ);
    float logStep = (logMax - logMin) / (NUM_BANDS - 1);

    for (int i = 0; i < NUM_BANDS; i++) {
        m_bandFreqs[i] = std::pow(10.0f, logMin + i * logStep);
    }
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

void SpectrumWindow::onRender(ID2D1RenderTarget* rt) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    // Clear background
    fillRect(0, 0, static_cast<float>(getWidth()), static_cast<float>(getHeight()),
             DAWColors::Background);

    // Draw title
    drawText(L"Spectrum Analyzer (20 Hz - 20 kHz)", 10, 10, DAWColors::TextPrimary);

    // Calculate bar dimensions
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

    // Draw frequency labels (every 5th band)
    for (int i = 0; i < NUM_BANDS; i += 5) {
        float x = margin + i * barWidth;
        float freq = m_bandFreqs[i];

        wchar_t label[32];
        if (freq < 1000.0f) {
            swprintf(label, 32, L"%.0f", freq);
        } else {
            swprintf(label, 32, L"%.1fk", freq / 1000.0f);
        }

        drawText(label, x, height - bottomMargin + 10, DAWColors::TextSecondary);
    }

    // Draw grid lines
    for (int i = 0; i <= 4; i++) {
        float y = topMargin + (usableHeight * i) / 4;
        drawLine(margin, y, width - margin, y, DAWColors::GridLine, 1.0f);

        // dB labels
        wchar_t dbLabel[16];
        int dB = -i * 15;  // 0, -15, -30, -45, -60 dB
        swprintf(dbLabel, 16, L"%d dB", dB);
        drawText(dbLabel, 5, y - 8, DAWColors::TextSecondary);
    }

    // Draw spectrum bars
    for (int i = 0; i < NUM_BANDS; i++) {
        float x = margin + i * barWidth + barSpacing / 2;
        float barHeight = m_bandValues[i] * usableHeight;
        float y = height - bottomMargin - barHeight;

        // Gradient color from blue (low) to red (high)
        float hue = (1.0f - m_bandValues[i]) * 0.6f;  // 0.6 = blue, 0 = red
        Color barColor;
        if (hue > 0.33f) {  // Blue to cyan
            barColor = Color(0.0f, (0.6f - hue) * 3.0f, 1.0f);
        } else if (hue > 0.16f) {  // Cyan to yellow
            barColor = Color((0.33f - hue) * 6.0f, 1.0f, (hue - 0.16f) * 6.0f);
        } else {  // Yellow to red
            barColor = Color(1.0f, hue * 6.0f, 0.0f);
        }

        fillRect(x, y, barActualWidth, barHeight, barColor);

        // Draw peak indicator
        if (m_bandPeaks[i] > 0.01f) {
            float peakY = height - bottomMargin - m_bandPeaks[i] * usableHeight;
            drawLine(x, peakY, x + barActualWidth, peakY,
                    DAWColors::WaveformPeak, 2.0f);
        }
    }
}

void SpectrumWindow::onResize(int width, int height) {
    invalidate();
}
