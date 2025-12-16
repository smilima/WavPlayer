#pragma once
#include "Track.h"
#include "AudioEngine.h"
#include <string>
#include <vector>
#include <memory>
#include <map>

class Project {
public:
    Project();
    ~Project() = default;

    // Project file operations
    bool save(const std::wstring& filename);
    bool load(const std::wstring& filename);
    void clear();

    // Project properties
    const std::wstring& getFilename() const { return m_filename; }
    void setFilename(const std::wstring& filename) { m_filename = filename; }
    bool hasFilename() const { return !m_filename.empty(); }
    
    bool isModified() const { return m_modified; }
    void setModified(bool modified = true) { m_modified = modified; }

    // Project settings
    double getBPM() const { return m_bpm; }
    void setBPM(double bpm) { m_bpm = bpm; m_modified = true; }
    
    double getSampleRate() const { return m_sampleRate; }
    void setSampleRate(double rate) { m_sampleRate = rate; }

    // Track management
    void addTrack(std::shared_ptr<Track> track);
    void removeTrack(size_t index);
    std::vector<std::shared_ptr<Track>>& getTracks() { return m_tracks; }
    const std::vector<std::shared_ptr<Track>>& getTracks() const { return m_tracks; }

    // Audio clip cache (maps file paths to loaded clips)
    std::shared_ptr<AudioClip> getOrLoadClip(const std::wstring& filepath);
    void removeClipFromCache(const std::wstring& filepath);  // NEW
    const std::map<std::wstring, std::shared_ptr<AudioClip>>& getClipCache() const { return m_clipCache; }

    // Project name (derived from filename or "Untitled")
    std::wstring getProjectName() const;

    // Check if any track has audio loaded (i.e., at least one region with a valid clip)
    bool hasAudioLoaded() const {
        for (const auto& track : m_tracks) {
            for (const auto& region : track->getRegions()) {
                if (region.clip) {
                    return true;
                }
            }
        }
        return false;
    }

    // File extension
    static constexpr const wchar_t* FILE_EXTENSION = L".austd";
    static constexpr const wchar_t* FILE_FILTER = L"Audio Studio Project (*.austd)\0*.austd\0All Files (*.*)\0*.*\0";

private:
    bool parseProjectFile(const std::wstring& content);
    std::wstring serializeProject() const;
    
    static std::wstring trim(const std::wstring& str);
    static std::vector<std::wstring> split(const std::wstring& str, wchar_t delimiter);

    std::wstring m_filename;
    bool m_modified = false;
    
    // Project settings
    double m_bpm = 120.0;
    double m_sampleRate = 44100.0;
    
    // Tracks
    std::vector<std::shared_ptr<Track>> m_tracks;
    
    // Clip cache
    std::map<std::wstring, std::shared_ptr<AudioClip>> m_clipCache;

    // Reverse mapping for fast clip->path lookup during serialization
    mutable std::map<AudioClip*, std::wstring> m_clipToPathCache;

    // File format version
    static constexpr int FILE_VERSION = 1;
};
