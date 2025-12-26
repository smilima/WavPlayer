#pragma once
#include "AudioEngine.h"
#include <string>
#include <memory>
#include <algorithm>

struct TrackRegion {
    std::shared_ptr<AudioClip> clip;
    double startTime = 0.0;      // Position on timeline (seconds)
    double clipOffset = 0.0;     // Offset within the clip (seconds)
    double duration = 0.0;       // Length of region (seconds)
    
    double endTime() const { return startTime + duration; }
};

class Track {
public:
    Track(const std::wstring& name = L"New Track");

    // Properties
    const std::wstring& getName() const { return m_name; }
    void setName(const std::wstring& name) { m_name = name; }
    
    float getVolume() const { return m_volume; }
    void setVolume(float volume);

    float getPan() const { return m_pan; }
    void setPan(float pan);

    // EQ controls (-12 to +12 dB)
    float getEQLow() const { return m_eqLow; }
    void setEQLow(float gain) { m_eqLow = std::clamp(gain, -12.0f, 12.0f); }

    float getEQMid() const { return m_eqMid; }
    void setEQMid(float gain) { m_eqMid = std::clamp(gain, -12.0f, 12.0f); }

    float getEQHigh() const { return m_eqHigh; }
    void setEQHigh(float gain) { m_eqHigh = std::clamp(gain, -12.0f, 12.0f); }

    bool isMuted() const { return m_muted; }
    void setMuted(bool muted) { m_muted = muted; }
    
    bool isSolo() const { return m_solo; }
    void setSolo(bool solo) { m_solo = solo; }
    
    bool isArmed() const { return m_armed; }
    void setArmed(bool armed) { m_armed = armed; }
    
    // Visibility - track is visible if explicitly shown OR has regions
    bool isVisible() const { return m_visible || !m_regions.empty(); }
    void setVisible(bool visible) { m_visible = visible; }

    // Visual properties
    int getHeight() const { return m_height; }
    void setHeight(int height) { m_height = std::max(60, height); }
    
    uint32_t getColor() const { return m_color; }
    void setColor(uint32_t color) { m_color = color; }

    // Regions
    void addRegion(const TrackRegion& region);
    void removeRegion(size_t index);
    const std::vector<TrackRegion>& getRegions() const { return m_regions; }
    std::vector<TrackRegion>& getRegions() { return m_regions; }

    // Get audio at a specific time (for mixing)
    void getAudioAtTime(double time, float* leftOut, float* rightOut,
                        uint32_t sampleRate) const;

    // Audio level metering (for VU meters)
    float getPeakLevel() const { return m_peakLevel; }
    void setPeakLevel(float level) { m_peakLevel = std::max(0.0f, std::min(1.0f, level)); }
    void updatePeakLevel(float level);  // Update with decay

private:
    void updateGains();  // Update cached gains when volume or pan changes

    std::wstring m_name;
    float m_volume = 1.0f;
    float m_pan = 0.0f;  // -1.0 (left) to 1.0 (right)
    float m_eqLow = 0.0f;   // -12.0 to +12.0 dB
    float m_eqMid = 0.0f;   // -12.0 to +12.0 dB
    float m_eqHigh = 0.0f;  // -12.0 to +12.0 dB
    bool m_muted = false;
    bool m_solo = false;
    bool m_armed = false;
    bool m_visible = false;  // Explicitly visible even without regions

    int m_height = 100;
    uint32_t m_color = 0xFF4A90D9;  // Default blue color (ARGB)

    std::vector<TrackRegion> m_regions;

    // Cached gain values (updated when volume or pan changes)
    mutable float m_cachedLeftGain = 1.0f;
    mutable float m_cachedRightGain = 1.0f;

    // Audio metering
    mutable float m_peakLevel = 0.0f;  // Current peak level (0.0 to 1.0)
};
