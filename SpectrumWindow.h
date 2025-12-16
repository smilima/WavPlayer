#pragma once
#include "D2DWindow.h"
#include <vector>
#include <complex>
#include <mutex>

class SpectrumWindow : public D2DWindow {
public:
    SpectrumWindow();
    ~SpectrumWindow() override;

    // Update spectrum with new audio samples
    void updateSpectrum(const float* samples, size_t sampleCount, int sampleRate);

    // Clear the spectrum display
    void clear();

protected:
    void onRender(ID2D1RenderTarget* rt) override;
    void onResize(int width, int height) override;

private:
    // FFT implementation
    void performFFT(const std::vector<float>& samples);
    void fft(std::vector<std::complex<float>>& buffer, bool inverse = false);

    // Calculate frequency bands from FFT results
    void calculateBands();

    // Constants
    static constexpr int FFT_SIZE = 4096;
    static constexpr int NUM_BANDS = 20;
    static constexpr float MIN_FREQ = 20.0f;
    static constexpr float MAX_FREQ = 20000.0f;

    // FFT and spectrum data
    std::vector<std::complex<float>> m_fftBuffer;
    std::vector<float> m_magnitudes;
    std::vector<float> m_bandValues;      // Current band values (0.0 to 1.0)
    std::vector<float> m_bandPeaks;       // Peak values for each band
    std::vector<float> m_bandFreqs;       // Center frequency for each band

    std::mutex m_dataMutex;
    int m_sampleRate = 44100;

    // Smoothing
    static constexpr float PEAK_DECAY = 0.95f;
    static constexpr float SMOOTHING = 0.7f;
};
