#include <gtest/gtest.h>
#include "../Project.h"

// Test Project construction
TEST(ProjectTest, Constructor) {
    Project project;
    EXPECT_FALSE(project.hasFilename());
    EXPECT_FALSE(project.isModified());
    EXPECT_DOUBLE_EQ(project.getBPM(), 120.0);
    EXPECT_DOUBLE_EQ(project.getSampleRate(), 44100.0);
    EXPECT_TRUE(project.getTracks().empty());
}

// Test Project filename
TEST(ProjectTest, Filename) {
    Project project;
    EXPECT_FALSE(project.hasFilename());
    EXPECT_TRUE(project.getFilename().empty());

    project.setFilename(L"C:\\Projects\\test.austd");
    EXPECT_TRUE(project.hasFilename());
    EXPECT_EQ(project.getFilename(), L"C:\\Projects\\test.austd");
}

// Test Project modified flag
TEST(ProjectTest, ModifiedFlag) {
    Project project;
    EXPECT_FALSE(project.isModified());

    project.setModified(true);
    EXPECT_TRUE(project.isModified());

    project.setModified(false);
    EXPECT_FALSE(project.isModified());
}

// Test Project BPM
TEST(ProjectTest, BPM) {
    Project project;
    EXPECT_DOUBLE_EQ(project.getBPM(), 120.0);

    project.setBPM(140.0);
    EXPECT_DOUBLE_EQ(project.getBPM(), 140.0);
    EXPECT_TRUE(project.isModified());  // BPM change should set modified flag
}

// Test Project sample rate
TEST(ProjectTest, SampleRate) {
    Project project;
    EXPECT_DOUBLE_EQ(project.getSampleRate(), 44100.0);

    project.setSampleRate(48000.0);
    EXPECT_DOUBLE_EQ(project.getSampleRate(), 48000.0);
}

// Test Project track management
TEST(ProjectTest, TrackManagement) {
    Project project;
    EXPECT_TRUE(project.getTracks().empty());

    auto track1 = std::make_shared<Track>(L"Track 1");
    auto track2 = std::make_shared<Track>(L"Track 2");

    project.addTrack(track1);
    EXPECT_EQ(project.getTracks().size(), 1);
    EXPECT_EQ(project.getTracks()[0]->getName(), L"Track 1");

    project.addTrack(track2);
    EXPECT_EQ(project.getTracks().size(), 2);
    EXPECT_EQ(project.getTracks()[1]->getName(), L"Track 2");

    project.removeTrack(0);
    EXPECT_EQ(project.getTracks().size(), 1);
    EXPECT_EQ(project.getTracks()[0]->getName(), L"Track 2");
}

// Test Project clear
TEST(ProjectTest, Clear) {
    Project project;
    project.setFilename(L"test.austd");
    project.setBPM(140.0);

    auto track = std::make_shared<Track>(L"Track 1");
    project.addTrack(track);

    EXPECT_FALSE(project.getTracks().empty());
    EXPECT_TRUE(project.hasFilename());

    project.clear();

    EXPECT_TRUE(project.getTracks().empty());
    EXPECT_FALSE(project.hasFilename());
    EXPECT_FALSE(project.isModified());
}

// Test Project name extraction
TEST(ProjectTest, ProjectName) {
    Project project;

    // No filename - should return "Untitled"
    EXPECT_EQ(project.getProjectName(), L"Untitled");

    // With filename
    project.setFilename(L"C:\\Projects\\MyProject.austd");
    EXPECT_EQ(project.getProjectName(), L"MyProject");

    // Different path style
    project.setFilename(L"MyProject.austd");
    EXPECT_EQ(project.getProjectName(), L"MyProject");
}

// Test Project hasAudioLoaded
TEST(ProjectTest, HasAudioLoaded) {
    Project project;
    EXPECT_FALSE(project.hasAudioLoaded());

    auto track = std::make_shared<Track>(L"Track 1");
    project.addTrack(track);

    // Track with no regions
    EXPECT_FALSE(project.hasAudioLoaded());

    // Add a region without a clip
    TrackRegion region;
    region.startTime = 0.0;
    region.duration = 5.0;
    track->addRegion(region);

    EXPECT_FALSE(project.hasAudioLoaded());  // Still no clip

    // Add a region with a clip
    TrackRegion regionWithClip;
    regionWithClip.clip = std::make_shared<AudioClip>();
    regionWithClip.startTime = 10.0;
    regionWithClip.duration = 3.0;
    track->addRegion(regionWithClip);

    EXPECT_TRUE(project.hasAudioLoaded());  // Now has audio
}

// Test Project file extension constants
TEST(ProjectTest, FileExtension) {
    EXPECT_STREQ(Project::FILE_EXTENSION, L".austd");
}
