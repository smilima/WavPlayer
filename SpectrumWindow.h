#pragma once
#include "D2DWindow.h"
#include <vector>
#include <complex>
#include <mutex>
#include <array>

class SpectrumWindow : public D2DWindow {
public:
    SpectrumWindow();
    ~SpectrumWindow() override;

    // Update spectrum with new audio samples
    void updateSpectrum(const float* samples, size_t sampleCount, int sampleRate);

    // Clear the spectrum display
    void clear();

    // Get EQ gains for audio processing (in dB, -12 to +12)
    const std::array<float, 12>& getEQGains() const { return m_eqGains; }

    // Apply EQ to stereo audio buffer (interleaved L/R)
    void applyEQ(float* samples, size_t frameCount, int sampleRate);

protected:
    void onRender(ID2D1RenderTarget* rt) override;
    void onResize(int width, int height) override;
    void onMouseDown(int x, int y, int button) override;
    void onMouseUp(int x, int y, int button) override;
    void onMouseMove(int x, int y) override;
    bool onClose() override { return true; }  // Hide instead of destroy

private:
    // FFT implementation
    void performFFT(const std::vector<float>& samples);
    void fft(std::vector<std::complex<float>>& buffer, bool inverse = false);

    // Calculate frequency bands from FFT results
    void calculateBands();

    // Biquad filter for each EQ band
    struct BiquadFilter {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;  // Numerator coefficients
        float a1 = 0.0f, a2 = 0.0f;              // Denominator coefficients
        float z1_L = 0.0f, z2_L = 0.0f;          // Left channel state
        float z1_R = 0.0f, z2_R = 0.0f;          // Right channel state

        void calculatePeakingEQ(float centerFreq, float gainDB, float Q, float sampleRate);
        float process(float input, float& z1, float& z2);
    };

    void updateFilters();
    int getSliderAtPosition(int x, int y);
    float getGainFromY(int y);

    // Constants
    static constexpr int FFT_SIZE = 4096;
    static constexpr int NUM_BANDS = 12;
    static constexpr float MIN_FREQ = 20.0f;
    static constexpr float MAX_FREQ = 20000.0f;

    // Standard ISO graphic EQ center frequencies (Hz)
    static constexpr std::array<float, 12> BAND_FREQUENCIES = {
        31.5f, 63.0f, 125.0f, 250.0f, 500.0f, 1000.0f,
        2000.0f, 4000.0f, 8000.0f, 16000.0f, 20000.0f, 20000.0f
    };

    // FFT and spectrum data
    std::vector<std::complex<float>> m_fftBuffer;
    std::vector<float> m_magnitudes;
    std::vector<float> m_bandValues;      // Current band values (0.0 to 1.0)
    std::vector<float> m_bandPeaks;       // Peak values for each band
    std::vector<float> m_bandFreqs;       // Center frequency for each band

    // EQ data
    std::array<float, NUM_BANDS> m_eqGains;           // Gain in dB (-12 to +12)
    std::array<BiquadFilter, NUM_BANDS> m_filters;    // One filter per band
    bool m_filtersInitialized = false;

    // UI interaction
    int m_draggedSlider = -1;
    bool m_isDragging = false;

    std::mutex m_dataMutex;
    int m_sampleRate = 44100;

    // Smoothing
    static constexpr float PEAK_DECAY = 0.95f;
    static constexpr float SMOOTHING = 0.7f;
    static constexpr float Q_FACTOR = 1.414f;  // Q for peaking filters
};
