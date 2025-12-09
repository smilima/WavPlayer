#pragma once
#include <Windows.h>
#include <mmsystem.h>
#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <cstdint>
#include <memory>

struct AudioFormat {
    uint16_t channels = 2;
    uint32_t sampleRate = 44100;
    uint16_t bitsPerSample = 16;
    
    uint32_t bytesPerSample() const { return bitsPerSample / 8; }
    uint32_t bytesPerFrame() const { return bytesPerSample() * channels; }
};

class AudioClip {
public:
    bool loadFromFile(const std::wstring& filename);
    
    const std::vector<float>& getSamples() const { return m_samples; }
    const AudioFormat& getFormat() const { return m_format; }
    double getDuration() const;
    size_t getSampleCount() const { return m_samples.size() / m_format.channels; }
    
    // Get min/max values for waveform display (returns pairs of min,max for each block)
    // startTime/endTime are in seconds relative to clip start
    std::vector<std::pair<float, float>> getWaveformData(size_t numBlocks, 
                                                          double startTime = 0.0, 
                                                          double endTime = -1.0) const;

private:
    std::vector<float> m_samples;  // Normalized to -1.0 to 1.0
    AudioFormat m_format;
    std::wstring m_filename;
};

class AudioEngine {
public:
    using PositionCallback = std::function<void(double positionSeconds)>;

    AudioEngine();
    ~AudioEngine();

    bool initialize(uint32_t sampleRate = 44100, uint16_t channels = 2);
    void shutdown();

    // Transport controls
    bool play();
    void pause();
    void stop();
    void setPosition(double seconds);
    
    double getPosition() const;
    double getDuration() const;
    bool isPlaying() const { return m_isPlaying; }

    // Clip management
    void setClip(std::shared_ptr<AudioClip> clip);
    std::shared_ptr<AudioClip> getClip() const { return m_clip; }

    // Volume (0.0 to 1.0)
    void setVolume(float volume);
    float getVolume() const { return m_volume; }

    // Callbacks
    void setPositionCallback(PositionCallback callback) { m_positionCallback = callback; }

    // Get current playback sample position
    size_t getPlaybackPosition() const { return m_playbackPosition; }

private:
    static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, 
                                      DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    void fillBuffer(WAVEHDR* header);
    void processAudio(int16_t* buffer, size_t frameCount);

    HWAVEOUT m_waveOut = nullptr;
    WAVEFORMATEX m_waveFormat = {};
    
    static constexpr int NUM_BUFFERS = 3;
    static constexpr int BUFFER_SIZE_FRAMES = 2048;
    WAVEHDR m_headers[NUM_BUFFERS] = {};
    std::vector<int16_t> m_buffers[NUM_BUFFERS];

    std::shared_ptr<AudioClip> m_clip;
    std::atomic<size_t> m_playbackPosition{0};
    std::atomic<bool> m_isPlaying{false};
    std::atomic<float> m_volume{1.0f};
    
    PositionCallback m_positionCallback;
};
