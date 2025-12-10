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
    void setVolume(float volume) { m_volume = std::clamp(volume, 0.0f, 1.0f); }
    
    float getPan() const { return m_pan; }
    void setPan(float pan) { m_pan = std::clamp(pan, -1.0f, 1.0f); }
    
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

private:
    std::wstring m_name;
    float m_volume = 1.0f;
    float m_pan = 0.0f;  // -1.0 (left) to 1.0 (right)
    bool m_muted = false;
    bool m_solo = false;
    bool m_armed = false;
    bool m_visible = false;  // Explicitly visible even without regions
    
    int m_height = 100;
    uint32_t m_color = 0xFF4A90D9;  // Default blue color (ARGB)
    
    std::vector<TrackRegion> m_regions;
};
