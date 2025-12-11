#include "AudioEngine.h"
#include "Track.h"
#include <fstream>
#include <algorithm>
#include <cmath>

// ============================================================================
// AudioClip Implementation
// ============================================================================

bool AudioClip::loadFromFile(const std::wstring& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    // Read RIFF header
    char riff[4];
    file.read(riff, 4);
    if (strncmp(riff, "RIFF", 4) != 0) return false;

    uint32_t fileSize;
    file.read(reinterpret_cast<char*>(&fileSize), 4);

    char wave[4];
    file.read(wave, 4);
    if (strncmp(wave, "WAVE", 4) != 0) return false;

    // Parse chunks
    std::vector<uint8_t> rawData;
    
    while (file) {
        char chunkId[4];
        uint32_t chunkSize;
        
        file.read(chunkId, 4);
        if (!file) break;
        
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        if (!file) break;

        if (strncmp(chunkId, "fmt ", 4) == 0) {
            uint16_t audioFormat;
            file.read(reinterpret_cast<char*>(&audioFormat), 2);
            file.read(reinterpret_cast<char*>(&m_format.channels), 2);
            file.read(reinterpret_cast<char*>(&m_format.sampleRate), 4);
            
            uint32_t byteRate;
            uint16_t blockAlign;
            file.read(reinterpret_cast<char*>(&byteRate), 4);
            file.read(reinterpret_cast<char*>(&blockAlign), 2);
            file.read(reinterpret_cast<char*>(&m_format.bitsPerSample), 2);
            
            // Skip any extra format bytes
            if (chunkSize > 16) {
                file.seekg(chunkSize - 16, std::ios::cur);
            }
        }
        else if (strncmp(chunkId, "data", 4) == 0) {
            rawData.resize(chunkSize);
            file.read(reinterpret_cast<char*>(rawData.data()), chunkSize);
            break;
        }
        else {
            // Skip unknown chunks
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    if (rawData.empty()) return false;

    // Convert to normalized float samples
    size_t sampleCount = rawData.size() / m_format.bytesPerSample();
    m_samples.resize(sampleCount);

    if (m_format.bitsPerSample == 16) {
        const int16_t* src = reinterpret_cast<const int16_t*>(rawData.data());
        for (size_t i = 0; i < sampleCount; ++i) {
            m_samples[i] = src[i] / 32768.0f;
        }
    }
    else if (m_format.bitsPerSample == 8) {
        const uint8_t* src = rawData.data();
        for (size_t i = 0; i < sampleCount; ++i) {
            m_samples[i] = (src[i] - 128) / 128.0f;
        }
    }
    else if (m_format.bitsPerSample == 24) {
        for (size_t i = 0; i < sampleCount; ++i) {
            int32_t sample = (rawData[i * 3] << 8) | 
                             (rawData[i * 3 + 1] << 16) | 
                             (rawData[i * 3 + 2] << 24);
            sample >>= 8;  // Sign extend
            m_samples[i] = sample / 8388608.0f;
        }
    }
    else if (m_format.bitsPerSample == 32) {
        const int32_t* src = reinterpret_cast<const int32_t*>(rawData.data());
        for (size_t i = 0; i < sampleCount; ++i) {
            m_samples[i] = src[i] / 2147483648.0f;
        }
    }

    m_filename = filename;
    return true;
}

bool AudioClip::saveToFile(const std::wstring& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;

    // Calculate sizes - each float sample becomes one int16 sample
    uint32_t dataSize = static_cast<uint32_t>(m_samples.size() * sizeof(int16_t));
    uint32_t fileSize = 36 + dataSize;

    // RIFF header
    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&fileSize), 4);
    file.write("WAVE", 4);

    // fmt chunk
    file.write("fmt ", 4);
    uint32_t fmtSize = 16;
    file.write(reinterpret_cast<const char*>(&fmtSize), 4);
    
    uint16_t audioFormat = 1;  // PCM
    uint16_t channels = m_format.channels;
    uint32_t sampleRate = m_format.sampleRate;
    uint16_t bitsPerSample = 16;  // Always save as 16-bit
    uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
    uint16_t blockAlign = channels * (bitsPerSample / 8);
    
    file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&channels), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data chunk
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataSize), 4);

    // Convert float samples to int16 and write
    for (size_t i = 0; i < m_samples.size(); ++i) {
        float sample = std::clamp(m_samples[i], -1.0f, 1.0f);
        int16_t intSample = static_cast<int16_t>(sample * 32767.0f);
        file.write(reinterpret_cast<const char*>(&intSample), 2);
    }

    return file.good();
}

double AudioClip::getDuration() const {
    if (m_format.sampleRate == 0 || m_format.channels == 0) return 0.0;
    return static_cast<double>(getSampleCount()) / m_format.sampleRate;
}

std::vector<std::pair<float, float>> AudioClip::getWaveformData(size_t numBlocks,
                                                                  double startTime,
                                                                  double endTime) const {
    std::vector<std::pair<float, float>> waveform(numBlocks);
    
    if (m_samples.empty() || numBlocks == 0 || m_format.sampleRate == 0) return waveform;

    size_t totalFrames = getSampleCount();
    double duration = getDuration();
    
    // Default endTime to full duration if not specified
    if (endTime < 0) {
        endTime = duration;
    }
    
    // Clamp time range to valid bounds
    startTime = std::max(0.0, std::min(startTime, duration));
    endTime = std::max(startTime, std::min(endTime, duration));
    
    // Convert time to frame indices
    size_t startFrame = static_cast<size_t>(startTime * m_format.sampleRate);
    size_t endFrame = static_cast<size_t>(endTime * m_format.sampleRate);
    
    startFrame = std::min(startFrame, totalFrames);
    endFrame = std::min(endFrame, totalFrames);
    
    if (endFrame <= startFrame) return waveform;
    
    size_t rangeFrames = endFrame - startFrame;
    size_t framesPerBlock = std::max<size_t>(1, rangeFrames / numBlocks);

    for (size_t block = 0; block < numBlocks; ++block) {
        size_t blockStart = startFrame + block * framesPerBlock;
        size_t blockEnd = std::min(blockStart + framesPerBlock, endFrame);
        
        if (blockStart >= endFrame) break;
        
        float minVal = 0.0f;
        float maxVal = 0.0f;

        for (size_t frame = blockStart; frame < blockEnd; ++frame) {
            // Average all channels for display
            float sample = 0.0f;
            for (uint16_t ch = 0; ch < m_format.channels; ++ch) {
                sample += m_samples[frame * m_format.channels + ch];
            }
            sample /= m_format.channels;

            minVal = std::min(minVal, sample);
            maxVal = std::max(maxVal, sample);
        }

        waveform[block] = {minVal, maxVal};
    }

    return waveform;
}

// ============================================================================
// AudioEngine Implementation
// ============================================================================

AudioEngine::AudioEngine() {
    // ... existing initialization ...
    m_inputMonitorBuffer.assign(INPUT_MONITOR_BUFFER_SIZE, 0.0f);
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initialize(uint32_t sampleRate, uint16_t channels) {
    m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    m_waveFormat.nChannels = channels;
    m_waveFormat.nSamplesPerSec = sampleRate;
    m_waveFormat.wBitsPerSample = 16;
    m_waveFormat.nBlockAlign = channels * (m_waveFormat.wBitsPerSample / 8);
    m_waveFormat.nAvgBytesPerSec = sampleRate * m_waveFormat.nBlockAlign;
    m_waveFormat.cbSize = 0;

    MMRESULT result = waveOutOpen(
        &m_waveOut,
        WAVE_MAPPER,
        &m_waveFormat,
        reinterpret_cast<DWORD_PTR>(waveOutProc),
        reinterpret_cast<DWORD_PTR>(this),
        CALLBACK_FUNCTION
    );

    if (result != MMSYSERR_NOERROR) {
        return false;
    }

    // Allocate and prepare buffers
    size_t bufferSizeBytes = BUFFER_SIZE_FRAMES * m_waveFormat.nBlockAlign;
    
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        m_buffers[i].resize(BUFFER_SIZE_FRAMES * m_waveFormat.nChannels);
        
        m_headers[i].lpData = reinterpret_cast<LPSTR>(m_buffers[i].data());
        m_headers[i].dwBufferLength = static_cast<DWORD>(bufferSizeBytes);
        m_headers[i].dwUser = i;
        m_headers[i].dwFlags = 0;
        
        waveOutPrepareHeader(m_waveOut, &m_headers[i], sizeof(WAVEHDR));
    }

    return true;
}

void AudioEngine::shutdown() {
    stop();
    stopRecording();
    shutdownRecording();

    if (m_waveOut) {
        for (int i = 0; i < NUM_BUFFERS; ++i) {
            waveOutUnprepareHeader(m_waveOut, &m_headers[i], sizeof(WAVEHDR));
        }
        waveOutClose(m_waveOut);
        m_waveOut = nullptr;
    }
}

bool AudioEngine::play() {
    if (!m_waveOut) {
        OutputDebugStringW(L"play() failed: m_waveOut is null\n");
        return false;
    }
    
    // Check if we have anything to play - either tracks with duration or a single clip
    if (m_duration <= 0.0 && !m_clip) {
        wchar_t buf[128];
        swprintf_s(buf, L"play() failed: duration=%.2f, clip=%s\n", 
                   m_duration, m_clip ? L"yes" : L"no");
        OutputDebugStringW(buf);
        return false;
    }
    
    if (m_isPlaying) return true;
    
    m_isPlaying = true;
    
    // If we were paused, just restart - buffers are already queued
    if (m_isPaused) {
        m_isPaused = false;
        waveOutRestart(m_waveOut);
        return true;
    }

    // Make sure device is not paused
    waveOutRestart(m_waveOut);
    
    // Queue fresh buffers
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        // Clear done flag before resubmitting
        m_headers[i].dwFlags &= ~WHDR_DONE;
        fillBuffer(&m_headers[i]);
        MMRESULT result = waveOutWrite(m_waveOut, &m_headers[i], sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            wchar_t buf[64];
            swprintf_s(buf, L"waveOutWrite failed with error %d\n", result);
            OutputDebugStringW(buf);
            m_isPlaying = false;
            return false;
        }
    }

    if (!m_isPaused) {
        m_playbackStartTime = std::chrono::steady_clock::now();
        m_playbackStarted = true;
    }
    
    return true;
}

void AudioEngine::pause() {
    if (m_isPlaying) {
        // Save current position before pausing
        m_playbackPosition = static_cast<size_t>(getPosition() * m_waveFormat.nSamplesPerSec);
        m_playbackStarted = false;
        m_isPlaying = false;
        m_isPaused = true;
        waveOutPause(m_waveOut);
    }
}

void AudioEngine::stop() {
    if (m_waveOut) {
        m_isPlaying = false;
        m_isPaused = false;
        waveOutReset(m_waveOut);
        // Restart from reset state so device is ready to play
        waveOutRestart(m_waveOut);
        m_playbackStarted = false;
        m_playbackPosition = 0;
    }
}

void AudioEngine::setPosition(double seconds) {
    size_t frame = static_cast<size_t>(seconds * m_waveFormat.nSamplesPerSec);
    
    if (m_isPlaying) {
        // Adjust start time to maintain correct position
        double currentPos = getPosition();
        double offset = seconds - currentPos;
        m_playbackStartTime += std::chrono::microseconds(static_cast<long long>(offset * 1e6));
    } else {
        m_playbackPosition = frame;
    }
}

double AudioEngine::getPosition() const {
    if (m_isPlaying && m_playbackStarted) {
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - m_playbackStartTime).count() / 1e6;
        return elapsed;
    } else {
        return static_cast<double>(m_playbackPosition.load()) / m_waveFormat.nSamplesPerSec;
    }
}

double AudioEngine::getDuration() const {
    // Use explicitly set duration if available
    if (m_duration > 0.0) {
        return m_duration;
    }
    // Fall back to single clip duration
    return m_clip ? m_clip->getDuration() : 0.0;
}

void AudioEngine::setClip(std::shared_ptr<AudioClip> clip) {
    bool wasPlaying = m_isPlaying;
    stop();
    m_clip = clip;
    if (wasPlaying && clip) {
        play();
    }
}

void AudioEngine::setVolume(float volume) {
    m_volume = std::clamp(volume, 0.0f, 1.0f);
}

void CALLBACK AudioEngine::waveOutProc(HWAVEOUT hwo, UINT uMsg,
                                        DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WOM_DONE) {
        AudioEngine* engine = reinterpret_cast<AudioEngine*>(dwInstance);
        WAVEHDR* header = reinterpret_cast<WAVEHDR*>(dwParam1);
        
        if (engine->m_isPlaying) {
            engine->fillBuffer(header);
            waveOutWrite(hwo, header, sizeof(WAVEHDR));
            
            // Notify position update
            if (engine->m_positionCallback) {
                engine->m_positionCallback(engine->getPosition());
            }
        }
    }
}

void AudioEngine::fillBuffer(WAVEHDR* header) {
    // Clear the done flag before resubmitting
    header->dwFlags &= ~WHDR_DONE;
    
    int bufferIndex = static_cast<int>(header->dwUser);
    int16_t* buffer = m_buffers[bufferIndex].data();
    
    processAudio(buffer, BUFFER_SIZE_FRAMES);
}

void AudioEngine::processAudio(int16_t* buffer, size_t frameCount) {
    float masterVolume = m_volume.load();
    size_t pos = m_playbackPosition.load();
    double sampleRate = static_cast<double>(m_waveFormat.nSamplesPerSec);
    double duration = getDuration();
    size_t totalFrames = static_cast<size_t>(duration * sampleRate);
    
    // If no duration, output silence
    if (totalFrames == 0 && !m_clip) {
        memset(buffer, 0, frameCount * m_waveFormat.nBlockAlign);
        return;
    }
    
    // Check if we have tracks to mix
    if (m_tracks && m_duration > 0.0) {
        // Check if any track is soloed
        bool hasSolo = false;
        for (const auto& track : *m_tracks) {
            if (track->isSolo() && track->isVisible()) {
                hasSolo = true;
                break;
            }
        }
        
        for (size_t frame = 0; frame < frameCount; ++frame) {
            double currentTime = static_cast<double>(pos + frame) / sampleRate;
            
            if (pos + frame >= totalFrames) {
                // End of project - output silence
                buffer[frame * m_waveFormat.nChannels] = 0;
                if (m_waveFormat.nChannels > 1) {
                    buffer[frame * m_waveFormat.nChannels + 1] = 0;
                }
            }
            else {
                float leftMix = 0.0f;
                float rightMix = 0.0f;
                
                // Mix audio from all tracks
                for (const auto& track : *m_tracks) {
                    if (!track->isVisible()) continue;
                    if (track->isMuted()) continue;
                    if (hasSolo && !track->isSolo()) continue;
                    
                    float left = 0.0f, right = 0.0f;
                    track->getAudioAtTime(currentTime, &left, &right, m_waveFormat.nSamplesPerSec);
                    leftMix += left;
                    rightMix += right;
                }
                
                // Mix in input monitoring if enabled
                if (m_inputMonitoring) {
                    float inputLeft = m_inputMonitorBuffer[m_inputMonitorReadPos];
                    m_inputMonitorReadPos = (m_inputMonitorReadPos + 1) % INPUT_MONITOR_BUFFER_SIZE;
                    float inputRight = (m_waveFormat.nChannels > 1) ? m_inputMonitorBuffer[m_inputMonitorReadPos] : inputLeft;
                    m_inputMonitorReadPos = (m_inputMonitorReadPos + 1) % INPUT_MONITOR_BUFFER_SIZE;
                    leftMix += inputLeft;
                    rightMix += inputRight;
                }
                
                // Apply master volume and clamp
                leftMix *= masterVolume;
                rightMix *= masterVolume;
                leftMix = std::clamp(leftMix, -1.0f, 1.0f);
                rightMix = std::clamp(rightMix, -1.0f, 1.0f);
                
                // Convert to int16
                buffer[frame * m_waveFormat.nChannels] = static_cast<int16_t>(leftMix * 32767.0f);
                if (m_waveFormat.nChannels > 1) {
                    buffer[frame * m_waveFormat.nChannels + 1] = static_cast<int16_t>(rightMix * 32767.0f);
                }
            }
        }
        
        // Advance playback position
        pos += frameCount;
        if (pos >= totalFrames) {
            m_isPlaying = false;
        }
        m_playbackPosition = pos;
    }
    // Fall back to single clip playback
    else if (m_clip) {
        const auto& samples = m_clip->getSamples();
        const auto& format = m_clip->getFormat();
        size_t totalFrames = m_clip->getSampleCount();

        for (size_t frame = 0; frame < frameCount; ++frame) {
            if (pos >= totalFrames) {
                // End of clip - output silence
                for (uint16_t ch = 0; ch < m_waveFormat.nChannels; ++ch) {
                    buffer[frame * m_waveFormat.nChannels + ch] = 0;
                }
            }
            else {
                // Copy and convert samples
                for (uint16_t ch = 0; ch < m_waveFormat.nChannels; ++ch) {
                    float sample = 0.0f;
                    
                    // Handle channel mapping (mono to stereo, etc.)
                    if (ch < format.channels) {
                        sample = samples[pos * format.channels + ch];
                    }
                    else if (format.channels > 0) {
                        sample = samples[pos * format.channels];  // Duplicate first channel
                    }
                    
                    // Mix in input monitoring if enabled
                    if (m_inputMonitoring) {
                        float inputSample = m_inputMonitorBuffer[m_inputMonitorReadPos];
                        m_inputMonitorReadPos = (m_inputMonitorReadPos + 1) % INPUT_MONITOR_BUFFER_SIZE;
                        sample += inputSample;
                    }
                    
                    // Apply volume and convert to int16
                    sample *= masterVolume;
                    sample = std::clamp(sample, -1.0f, 1.0f);
                    buffer[frame * m_waveFormat.nChannels + ch] = 
                        static_cast<int16_t>(sample * 32767.0f);
                }
                ++pos;
            }
        }
        
        // Update playback position
        m_playbackPosition = pos;
    }
    else {
        // No audio source - output silence, but still monitor input if enabled
        if (m_inputMonitoring) {
            for (size_t frame = 0; frame < frameCount; ++frame) {
                for (uint16_t ch = 0; ch < m_waveFormat.nChannels; ++ch) {
                    float inputSample = m_inputMonitorBuffer[m_inputMonitorReadPos];
                    m_inputMonitorReadPos = (m_inputMonitorReadPos + 1) % INPUT_MONITOR_BUFFER_SIZE;
                    inputSample *= masterVolume;
                    inputSample = std::clamp(inputSample, -1.0f, 1.0f);
                    buffer[frame * m_waveFormat.nChannels + ch] = static_cast<int16_t>(inputSample * 32767.0f);
                }
            }
        } else {
            memset(buffer, 0, frameCount * m_waveFormat.nBlockAlign);
        }
    }
}

// ============================================================================
// Recording Implementation
// ============================================================================

std::vector<std::wstring> AudioEngine::getInputDevices() {
    std::vector<std::wstring> devices;
    
    UINT numDevices = waveInGetNumDevs();
    for (UINT i = 0; i < numDevices; ++i) {
        WAVEINCAPS caps;
        if (waveInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            devices.push_back(caps.szPname);
        }
    }
    
    return devices;
}

bool AudioEngine::setInputDevice(int deviceIndex) {
    if (m_isRecording) return false;
    
    UINT numDevices = waveInGetNumDevs();
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(numDevices)) {
        return false;
    }
    
    m_inputDeviceIndex = deviceIndex;
    return true;
}

bool AudioEngine::initializeRecording() {
    if (m_waveIn) return true;  // Already initialized
    
    MMRESULT result = waveInOpen(
        &m_waveIn,
        m_inputDeviceIndex,
        &m_waveFormat,
        reinterpret_cast<DWORD_PTR>(waveInProc),
        reinterpret_cast<DWORD_PTR>(this),
        CALLBACK_FUNCTION
    );
    
    if (result != MMSYSERR_NOERROR) {
        m_waveIn = nullptr;
        return false;
    }
    
    // Allocate and prepare recording buffers
    size_t bufferSizeBytes = RECORD_BUFFER_SIZE_FRAMES * m_waveFormat.nBlockAlign;
    
    for (int i = 0; i < NUM_RECORD_BUFFERS; ++i) {
        m_recordBuffers[i].resize(RECORD_BUFFER_SIZE_FRAMES * m_waveFormat.nChannels);
        
        m_recordHeaders[i].lpData = reinterpret_cast<LPSTR>(m_recordBuffers[i].data());
        m_recordHeaders[i].dwBufferLength = static_cast<DWORD>(bufferSizeBytes);
        m_recordHeaders[i].dwUser = i;
        m_recordHeaders[i].dwFlags = 0;
        m_recordHeaders[i].dwBytesRecorded = 0;
        
        waveInPrepareHeader(m_waveIn, &m_recordHeaders[i], sizeof(WAVEHDR));
    }
    
    return true;
}

void AudioEngine::shutdownRecording() {
    if (m_waveIn) {
        waveInReset(m_waveIn);
        
        for (int i = 0; i < NUM_RECORD_BUFFERS; ++i) {
            waveInUnprepareHeader(m_waveIn, &m_recordHeaders[i], sizeof(WAVEHDR));
        }
        
        waveInClose(m_waveIn);
        m_waveIn = nullptr;
    }
}

bool AudioEngine::startRecording() {
    if (m_isRecording) return true;
    
    // Initialize recording device if needed
    if (!initializeRecording()) {
        return false;
    }
    
    // Clear any previous recorded data
    {
        std::lock_guard<std::mutex> lock(m_recordMutex);
        m_recordedSamples.clear();
    }
    
    // If input monitoring is enabled and we're not already playing, start playback to hear tracks
    if (m_inputMonitoring && !m_isPlaying && (m_duration > 0.0 || m_clip)) {
        play();
    }
    
    // Queue all recording buffers
    for (int i = 0; i < NUM_RECORD_BUFFERS; ++i) {
        m_recordHeaders[i].dwBytesRecorded = 0;
        waveInAddBuffer(m_waveIn, &m_recordHeaders[i], sizeof(WAVEHDR));
    }
    
    // Start recording
    MMRESULT result = waveInStart(m_waveIn);
    if (result != MMSYSERR_NOERROR) {
        return false;
    }
    
    m_isRecording = true;
    return true;
}

void AudioEngine::stopRecording() {
    if (!m_isRecording || !m_waveIn) return;
    
    // Set stopping flag - callbacks will write to pending buffer
    m_isStopping = true;
    
    // Clear pending samples buffer (no lock needed, we own it during stopping)
    m_pendingSamples.clear();
    
    // Stop recording
    waveInStop(m_waveIn);
    
    // Reset returns all buffers - they go to m_pendingSamples
    waveInReset(m_waveIn);
    
    // Now merge pending samples into recorded samples
    {
        std::lock_guard<std::mutex> lock(m_recordMutex);
        m_recordedSamples.insert(m_recordedSamples.end(), 
                                  m_pendingSamples.begin(), 
                                  m_pendingSamples.end());
    }
    
    // Now safe to clear flags
    m_isRecording = false;
    m_isStopping = false;
    
    // Notify callback if set
    if (m_recordingCallback) {
        auto clip = getRecordedClip();
        if (clip) {
            m_recordingCallback(clip);
        }
    }
}

std::shared_ptr<AudioClip> AudioEngine::getRecordedClip() {
    std::lock_guard<std::mutex> lock(m_recordMutex);
    
    if (m_recordedSamples.empty()) {
        return nullptr;
    }
    
    auto clip = std::make_shared<AudioClip>();
    
    AudioFormat format;
    format.channels = m_waveFormat.nChannels;
    format.sampleRate = m_waveFormat.nSamplesPerSec;
    format.bitsPerSample = m_waveFormat.wBitsPerSample;
    clip->setFormat(format);
    
    clip->getSamplesWritable() = m_recordedSamples;
    
    return clip;
}

double AudioEngine::getRecordingDuration() const {
    if (m_waveFormat.nSamplesPerSec == 0 || m_waveFormat.nChannels == 0) {
        return 0.0;
    }
    
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_recordMutex));
    size_t frameCount = m_recordedSamples.size() / m_waveFormat.nChannels;
    return static_cast<double>(frameCount) / m_waveFormat.nSamplesPerSec;
}

void CALLBACK AudioEngine::waveInProc(HWAVEIN hwi, UINT uMsg,
                                       DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WIM_DATA) {
        AudioEngine* engine = reinterpret_cast<AudioEngine*>(dwInstance);
        WAVEHDR* header = reinterpret_cast<WAVEHDR*>(dwParam1);
        
        // Always process the buffer to capture all audio data
        engine->processRecordedBuffer(header);
        
        // Only re-queue if actively recording (not stopping or stopped)
        if (engine->m_isRecording && !engine->m_isStopping && engine->m_waveIn) {
            header->dwBytesRecorded = 0;
            waveInAddBuffer(hwi, header, sizeof(WAVEHDR));
        }
    }
}

void AudioEngine::processRecordedBuffer(WAVEHDR* header) {
    if (!header || header->dwBytesRecorded == 0) return;
    
    const int16_t* inputBuffer = reinterpret_cast<const int16_t*>(header->lpData);
    size_t sampleCount = header->dwBytesRecorded / sizeof(int16_t);
    
    // During stopping phase, write directly to pending buffer (no lock needed)
    if (m_isStopping) {
        m_pendingSamples.reserve(m_pendingSamples.size() + sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) {
            float sample = inputBuffer[i] / 32768.0f;
            m_pendingSamples.push_back(sample);
        }
        return;
    }
    
    // Normal recording - use mutex
    std::lock_guard<std::mutex> lock(m_recordMutex);
    
    m_recordedSamples.reserve(m_recordedSamples.size() + sampleCount);
    
    for (size_t i = 0; i < sampleCount; ++i) {
        float sample = inputBuffer[i] / 32768.0f;
        m_recordedSamples.push_back(sample);
        
        if (m_inputMonitoring) {
            size_t writePos = m_inputMonitorWritePos.load();
            m_inputMonitorBuffer[writePos] = sample;
            m_inputMonitorWritePos = (writePos + 1) % INPUT_MONITOR_BUFFER_SIZE;
        }
    }
}
