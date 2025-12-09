#include "AudioEngine.h"
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

AudioEngine::AudioEngine() {}

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

    if (m_waveOut) {
        for (int i = 0; i < NUM_BUFFERS; ++i) {
            waveOutUnprepareHeader(m_waveOut, &m_headers[i], sizeof(WAVEHDR));
        }
        waveOutClose(m_waveOut);
        m_waveOut = nullptr;
    }
}

bool AudioEngine::play() {
    if (!m_waveOut || !m_clip) return false;
    
    if (m_isPlaying) return true;
    
    m_isPlaying = true;

    // Queue all buffers
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        fillBuffer(&m_headers[i]);
        waveOutWrite(m_waveOut, &m_headers[i], sizeof(WAVEHDR));
    }

    return true;
}

void AudioEngine::pause() {
    if (m_waveOut && m_isPlaying) {
        waveOutPause(m_waveOut);
        m_isPlaying = false;
    }
}

void AudioEngine::stop() {
    if (m_waveOut) {
        m_isPlaying = false;
        waveOutReset(m_waveOut);
        m_playbackPosition = 0;
    }
}

void AudioEngine::setPosition(double seconds) {
    if (m_clip) {
        size_t frame = static_cast<size_t>(seconds * m_waveFormat.nSamplesPerSec);
        m_playbackPosition = std::min(frame, m_clip->getSampleCount());
    }
}

double AudioEngine::getPosition() const {
    return static_cast<double>(m_playbackPosition.load()) / m_waveFormat.nSamplesPerSec;
}

double AudioEngine::getDuration() const {
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
    int bufferIndex = static_cast<int>(header->dwUser);
    int16_t* buffer = m_buffers[bufferIndex].data();
    
    processAudio(buffer, BUFFER_SIZE_FRAMES);
}

void AudioEngine::processAudio(int16_t* buffer, size_t frameCount) {
    if (!m_clip) {
        memset(buffer, 0, frameCount * m_waveFormat.nBlockAlign);
        return;
    }

    const auto& samples = m_clip->getSamples();
    const auto& format = m_clip->getFormat();
    size_t totalFrames = m_clip->getSampleCount();
    float volume = m_volume.load();

    size_t pos = m_playbackPosition.load();
    
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
                
                // Apply volume and convert to int16
                sample *= volume;
                sample = std::clamp(sample, -1.0f, 1.0f);
                buffer[frame * m_waveFormat.nChannels + ch] = 
                    static_cast<int16_t>(sample * 32767.0f);
            }
            ++pos;
        }
    }
    
    m_playbackPosition = pos;

    // Check if playback finished
    if (pos >= totalFrames) {
        m_isPlaying = false;
    }
}
