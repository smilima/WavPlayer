#include "Track.h"
#include <cmath>

Track::Track(const std::wstring& name) : m_name(name) {}

void Track::addRegion(const TrackRegion& region) {
    m_regions.push_back(region);
    
    // Sort regions by start time
    std::sort(m_regions.begin(), m_regions.end(), 
        [](const TrackRegion& a, const TrackRegion& b) {
            return a.startTime < b.startTime;
        });
}

void Track::removeRegion(size_t index) {
    if (index < m_regions.size()) {
        m_regions.erase(m_regions.begin() + index);
    }
}

void Track::getAudioAtTime(double time, float* leftOut, float* rightOut,
                           uint32_t sampleRate) const {
    *leftOut = 0.0f;
    *rightOut = 0.0f;
    
    // Skip muted tracks or armed tracks (to avoid feedback during recording)
    if (m_muted || m_armed) return;
    
    for (const auto& region : m_regions) {
        if (time >= region.startTime && time < region.endTime()) {
            double clipTime = region.clipOffset + (time - region.startTime);
            
            if (region.clip) {
                const auto& samples = region.clip->getSamples();
                const auto& format = region.clip->getFormat();
                
                size_t frameIndex = static_cast<size_t>(clipTime * format.sampleRate);
                if (frameIndex < region.clip->getSampleCount()) {
                    float left = samples[frameIndex * format.channels];
                    float right = (format.channels > 1) ? 
                                  samples[frameIndex * format.channels + 1] : left;
                    
                    // Apply track volume and pan
                    float leftGain = m_volume * std::min(1.0f, 1.0f - m_pan);
                    float rightGain = m_volume * std::min(1.0f, 1.0f + m_pan);
                    
                    *leftOut += left * leftGain;
                    *rightOut += right * rightGain;
                }
            }
        }
    }
}
