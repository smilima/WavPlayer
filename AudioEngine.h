#pragma once
#include <Windows.h>
#include <mmsystem.h>
#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <chrono>

// Forward declaration
class Track;

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
    bool saveToFile(const std::wstring& filename) const;
    
    const std::vector<float>& getSamples() const { return m_samples; }
    std::vector<float>& getSamplesWritable() { return m_samples; }
    const AudioFormat& getFormat() const { return m_format; }
    void setFormat(const AudioFormat& format) { m_format = format; }
    double getDuration() const;
    size_t getSampleCount() const { return m_samples.size() / m_format.channels; }
    
    // Get min/max values for waveform display (returns pairs of min,max for each block)
    // startTime/endTime are in seconds relative to clip start
    std::vector<std::pair<float, float>> getWaveformData(size_t numBlocks,
                                                          double startTime = 0.0,
                                                          double endTime = -1.0) const;

    // Invalidate waveform cache (call when audio data changes)
    void invalidateWaveformCache() const;

private:
    std::vector<float> m_samples;  // Normalized to -1.0 to 1.0
    AudioFormat m_format;
    std::wstring m_filename;

    // Waveform cache for performance
    mutable struct WaveformCache {
        std::vector<std::pair<float, float>> data;
        size_t numBlocks = 0;
        double startTime = 0.0;
        double endTime = 0.0;
        bool valid = false;
    } m_waveformCache;
};

class AudioEngine {
public:
    using PositionCallback = std::function<void(double positionSeconds)>;
    using RecordingCallback = std::function<void(std::shared_ptr<AudioClip> recordedClip)>;

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

    // Recording controls
    bool startRecording();
    void stopRecording();
    bool isRecording() const { return m_isRecording; }
    std::shared_ptr<AudioClip> getRecordedClip();
    
    // Get list of available input devices
    static std::vector<std::wstring> getInputDevices();
    bool setInputDevice(int deviceIndex);
    int getInputDevice() const { return m_inputDeviceIndex; }

    // Track management for mixing
    void setTracks(std::vector<std::shared_ptr<Track>>* tracks) { m_tracks = tracks; }
    void setDuration(double duration) { m_duration = duration; }
    
    // Legacy single clip support (for simple playback)
    void setClip(std::shared_ptr<AudioClip> clip);
    std::shared_ptr<AudioClip> getClip() const { return m_clip; }

    // Master volume (0.0 to 1.0)
    void setVolume(float volume);
    float getVolume() const { return m_volume; }
    
    // Input monitoring
    void setInputMonitoring(bool enabled) { m_inputMonitoring = enabled; }
    bool getInputMonitoring() const { return m_inputMonitoring; }

    // Callbacks
    void setPositionCallback(PositionCallback callback) { m_positionCallback = callback; }
    void setRecordingCallback(RecordingCallback callback) { m_recordingCallback = callback; }

    // Get current playback sample position
    size_t getPlaybackPosition() const { return m_playbackPosition; }
    
    // Get sample rate
    uint32_t getSampleRate() const { return m_waveFormat.nSamplesPerSec; }
    
    // Get recording duration in seconds
    double getRecordingDuration() const;

private:
    // Playback
    static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, 
                                      DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    void fillBuffer(WAVEHDR* header);
    void processAudio(int16_t* buffer, size_t frameCount);

    // Recording
    static void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg,
                                     DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    void processRecordedBuffer(WAVEHDR* header);
    bool initializeRecording();
    void shutdownRecording();

    // Playback members
    HWAVEOUT m_waveOut = nullptr;
    WAVEFORMATEX m_waveFormat = {};
    
    static constexpr int NUM_BUFFERS = 3;
    static constexpr int BUFFER_SIZE_FRAMES = 2048;
    WAVEHDR m_headers[NUM_BUFFERS] = {};
    std::vector<int16_t> m_buffers[NUM_BUFFERS];

    std::shared_ptr<AudioClip> m_clip;
    std::vector<std::shared_ptr<Track>>* m_tracks = nullptr;  // Pointer to tracks for mixing
    double m_duration = 0.0;  // Total project duration
    std::atomic<size_t> m_playbackPosition{0};
    std::atomic<bool> m_isPlaying{false};
    std::atomic<bool> m_isPaused{false};  // True if paused (vs stopped)
    std::atomic<float> m_volume{1.0f};
    
    // Recording members
    HWAVEIN m_waveIn = nullptr;
    static constexpr int NUM_RECORD_BUFFERS = 4;
    static constexpr int RECORD_BUFFER_SIZE_FRAMES = 4096;
    WAVEHDR m_recordHeaders[NUM_RECORD_BUFFERS] = {};
    std::vector<int16_t> m_recordBuffers[NUM_RECORD_BUFFERS];
    
    std::vector<float> m_recordedSamples;
    std::vector<float> m_pendingSamples;  // NEW: Buffer for samples during stop
    mutable std::mutex m_recordMutex;
    std::atomic<bool> m_isRecording{false};
    std::atomic<bool> m_isStopping{false};
    std::atomic<bool> m_inputMonitoring{false};
    int m_inputDeviceIndex = 0;  // WAVE_MAPPER by default
    
    PositionCallback m_positionCallback;
    RecordingCallback m_recordingCallback;

    std::chrono::steady_clock::time_point m_playbackStartTime;
    bool m_playbackStarted = false;

    // Input monitoring buffer for real-time audio
    std::vector<float> m_inputMonitorBuffer;
    std::atomic<size_t> m_inputMonitorWritePos{0};
    size_t m_inputMonitorReadPos = 0;
    static constexpr size_t INPUT_MONITOR_BUFFER_SIZE = 8192;
};
