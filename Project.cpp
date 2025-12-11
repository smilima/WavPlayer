#include "Project.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <filesystem>

Project::Project() {
    // Create a default track
    auto track = std::make_shared<Track>(L"Track 1");
    track->setColor(0xFF4A90D9);
    m_tracks.push_back(track);
}

void Project::clear() {
    m_tracks.clear();
    m_clipCache.clear();
    m_filename.clear();
    m_modified = false;
    m_bpm = 120.0;
    m_sampleRate = 44100.0;
}

void Project::addTrack(std::shared_ptr<Track> track) {
    m_tracks.push_back(track);
    m_modified = true;
}

void Project::removeTrack(size_t index) {
    if (index < m_tracks.size()) {
        m_tracks.erase(m_tracks.begin() + index);
        m_modified = true;
    }
}

std::shared_ptr<AudioClip> Project::getOrLoadClip(const std::wstring& filepath) {
    // Check if already loaded
    auto it = m_clipCache.find(filepath);
    if (it != m_clipCache.end()) {
        return it->second;
    }
    
    // Load the clip
    auto clip = std::make_shared<AudioClip>();
    if (clip->loadFromFile(filepath)) {
        m_clipCache[filepath] = clip;
        return clip;
    }
    
    return nullptr;
}

void Project::removeClipFromCache(const std::wstring& filepath) {
    m_clipCache.erase(filepath);
}

std::wstring Project::getProjectName() const {
    if (m_filename.empty()) {
        return L"Untitled";
    }
    
    // Extract filename without path and extension
    std::filesystem::path path(m_filename);
    return path.stem().wstring();
}

bool Project::save(const std::wstring& filename) {
    std::wstring content = serializeProject();
    
    std::wofstream file(filename, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    file.close();
    
    if (file.fail()) {
        return false;
    }
    
    m_filename = filename;
    m_modified = false;
    return true;
}

bool Project::load(const std::wstring& filename) {
    std::wifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::wstringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::wstring content = buffer.str();
    
    if (!parseProjectFile(content)) {
        return false;
    }
    
    m_filename = filename;
    m_modified = false;
    return true;
}

std::wstring Project::serializeProject() const {
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(6);
    
    // Project header
    ss << L"[Project]\n";
    ss << L"Version=" << FILE_VERSION << L"\n";
    ss << L"BPM=" << m_bpm << L"\n";
    ss << L"SampleRate=" << m_sampleRate << L"\n";
    ss << L"\n";
    
    // Tracks
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        const auto& track = m_tracks[i];
        
        ss << L"[Track:" << i << L"]\n";
        ss << L"Name=" << track->getName() << L"\n";
        ss << L"Color=" << std::hex << std::uppercase << track->getColor() << std::dec << L"\n";
        ss << L"Volume=" << track->getVolume() << L"\n";
        ss << L"Pan=" << track->getPan() << L"\n";
        ss << L"Muted=" << (track->isMuted() ? 1 : 0) << L"\n";
        ss << L"Solo=" << (track->isSolo() ? 1 : 0) << L"\n";
        ss << L"Armed=" << (track->isArmed() ? 1 : 0) << L"\n";
        ss << L"Visible=" << (track->isVisible() ? 1 : 0) << L"\n";
        ss << L"Height=" << track->getHeight() << L"\n";
        ss << L"\n";
        
        // Track regions
        const auto& regions = track->getRegions();
        for (size_t r = 0; r < regions.size(); ++r) {
            const auto& region = regions[r];
            
            ss << L"[Region:" << i << L":" << r << L"]\n";
            
            // Find the clip path from the cache
            std::wstring clipPath;
            for (const auto& [path, clip] : m_clipCache) {
                if (clip == region.clip) {
                    clipPath = path;
                    break;
                }
            }
            
            ss << L"ClipPath=" << clipPath << L"\n";
            ss << L"StartTime=" << region.startTime << L"\n";
            ss << L"ClipOffset=" << region.clipOffset << L"\n";
            ss << L"Duration=" << region.duration << L"\n";
            ss << L"\n";
        }
    }
    
    return ss.str();
}

bool Project::parseProjectFile(const std::wstring& content) {
    // Clear existing data
    m_tracks.clear();
    m_clipCache.clear();
    
    std::wistringstream stream(content);
    std::wstring line;
    std::wstring currentSection;
    int currentTrackIndex = -1;
    int currentRegionTrack = -1;
    int currentRegionIndex = -1;
    
    std::map<std::wstring, std::wstring> sectionData;
    
    auto processPreviousSection = [&]() {
        if (currentSection == L"Project") {
            // Process project settings
            if (sectionData.count(L"BPM")) {
                m_bpm = std::stod(sectionData[L"BPM"]);
            }
            if (sectionData.count(L"SampleRate")) {
                m_sampleRate = std::stod(sectionData[L"SampleRate"]);
            }
        }
        else if (currentSection.find(L"Track:") == 0) {
            // Process track
            auto track = std::make_shared<Track>();
            
            if (sectionData.count(L"Name")) {
                track->setName(sectionData[L"Name"]);
            }
            if (sectionData.count(L"Color")) {
                track->setColor(std::stoul(sectionData[L"Color"], nullptr, 16));
            }
            if (sectionData.count(L"Volume")) {
                track->setVolume(std::stof(sectionData[L"Volume"]));
            }
            if (sectionData.count(L"Pan")) {
                track->setPan(std::stof(sectionData[L"Pan"]));
            }
            if (sectionData.count(L"Muted")) {
                track->setMuted(sectionData[L"Muted"] == L"1");
            }
            if (sectionData.count(L"Solo")) {
                track->setSolo(sectionData[L"Solo"] == L"1");
            }
            if (sectionData.count(L"Armed")) {
                track->setArmed(sectionData[L"Armed"] == L"1");
            }
            if (sectionData.count(L"Visible")) {
                track->setVisible(sectionData[L"Visible"] == L"1");
            }
            if (sectionData.count(L"Height")) {
                track->setHeight(std::stoi(sectionData[L"Height"]));
            }
            
            m_tracks.push_back(track);
        }
        else if (currentSection.find(L"Region:") == 0) {
            // Parse region section header "Region:trackIndex:regionIndex"
            auto parts = split(currentSection, L':');
            if (parts.size() >= 2) {
                int trackIdx = std::stoi(parts[1]);
                
                if (trackIdx >= 0 && trackIdx < static_cast<int>(m_tracks.size())) {
                    TrackRegion region;
                    
                    if (sectionData.count(L"ClipPath") && !sectionData[L"ClipPath"].empty()) {
                        region.clip = getOrLoadClip(sectionData[L"ClipPath"]);
                    }
                    if (sectionData.count(L"StartTime")) {
                        region.startTime = std::stod(sectionData[L"StartTime"]);
                    }
                    if (sectionData.count(L"ClipOffset")) {
                        region.clipOffset = std::stod(sectionData[L"ClipOffset"]);
                    }
                    if (sectionData.count(L"Duration")) {
                        region.duration = std::stod(sectionData[L"Duration"]);
                    }
                    
                    if (region.clip) {
                        m_tracks[trackIdx]->addRegion(region);
                    }
                }
            }
        }
        
        sectionData.clear();
    };
    
    while (std::getline(stream, line)) {
        line = trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == L';' || line[0] == L'#') {
            continue;
        }
        
        // Section header
        if (line[0] == L'[' && line.back() == L']') {
            // Process previous section before starting new one
            processPreviousSection();
            
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Key=Value pair
        size_t equalPos = line.find(L'=');
        if (equalPos != std::wstring::npos) {
            std::wstring key = trim(line.substr(0, equalPos));
            std::wstring value = trim(line.substr(equalPos + 1));
            sectionData[key] = value;
        }
    }
    
    // Process the last section
    processPreviousSection();
    
    // If no tracks were loaded, create a default one
    if (m_tracks.empty()) {
        auto track = std::make_shared<Track>(L"Track 1");
        track->setColor(0xFF4A90D9);
        m_tracks.push_back(track);
    }
    
    return true;
}

std::wstring Project::trim(const std::wstring& str) {
    const wchar_t* whitespace = L" \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::wstring::npos) return L"";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::vector<std::wstring> Project::split(const std::wstring& str, wchar_t delimiter) {
    std::vector<std::wstring> parts;
    std::wstringstream ss(str);
    std::wstring part;
    while (std::getline(ss, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}
