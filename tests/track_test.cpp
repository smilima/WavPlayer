#include <gtest/gtest.h>
#include "../Track.h"

// Test Track construction and basic properties
TEST(TrackTest, Constructor) {
    Track track(L"Test Track");
    EXPECT_EQ(track.getName(), L"Test Track");
    EXPECT_FLOAT_EQ(track.getVolume(), 1.0f);
    EXPECT_FLOAT_EQ(track.getPan(), 0.0f);
    EXPECT_FALSE(track.isMuted());
    EXPECT_FALSE(track.isSolo());
    EXPECT_FALSE(track.isArmed());
    EXPECT_EQ(track.getHeight(), 100);
    EXPECT_EQ(track.getColor(), 0xFF4A90D9);
}

// Test Track default constructor
TEST(TrackTest, DefaultConstructor) {
    Track track;
    EXPECT_EQ(track.getName(), L"New Track");
}

// Test Track name setter
TEST(TrackTest, SetName) {
    Track track;
    track.setName(L"My Custom Track");
    EXPECT_EQ(track.getName(), L"My Custom Track");
}

// Test Track volume
TEST(TrackTest, Volume) {
    Track track;
    track.setVolume(0.5f);
    EXPECT_FLOAT_EQ(track.getVolume(), 0.5f);

    track.setVolume(0.0f);
    EXPECT_FLOAT_EQ(track.getVolume(), 0.0f);

    track.setVolume(2.0f);
    EXPECT_FLOAT_EQ(track.getVolume(), 2.0f);
}

// Test Track pan
TEST(TrackTest, Pan) {
    Track track;
    track.setPan(-1.0f);  // Full left
    EXPECT_FLOAT_EQ(track.getPan(), -1.0f);

    track.setPan(1.0f);   // Full right
    EXPECT_FLOAT_EQ(track.getPan(), 1.0f);

    track.setPan(0.0f);   // Center
    EXPECT_FLOAT_EQ(track.getPan(), 0.0f);
}

// Test Track mute
TEST(TrackTest, Mute) {
    Track track;
    EXPECT_FALSE(track.isMuted());

    track.setMuted(true);
    EXPECT_TRUE(track.isMuted());

    track.setMuted(false);
    EXPECT_FALSE(track.isMuted());
}

// Test Track solo
TEST(TrackTest, Solo) {
    Track track;
    EXPECT_FALSE(track.isSolo());

    track.setSolo(true);
    EXPECT_TRUE(track.isSolo());

    track.setSolo(false);
    EXPECT_FALSE(track.isSolo());
}

// Test Track armed state
TEST(TrackTest, Armed) {
    Track track;
    EXPECT_FALSE(track.isArmed());

    track.setArmed(true);
    EXPECT_TRUE(track.isArmed());

    track.setArmed(false);
    EXPECT_FALSE(track.isArmed());
}

// Test Track visibility
TEST(TrackTest, Visibility) {
    Track track;
    EXPECT_FALSE(track.isVisible());  // Not visible by default

    track.setVisible(true);
    EXPECT_TRUE(track.isVisible());

    track.setVisible(false);
    EXPECT_FALSE(track.isVisible());
}

// Test Track visibility with regions
TEST(TrackTest, VisibilityWithRegions) {
    Track track;
    EXPECT_FALSE(track.isVisible());

    // Add a region (track becomes visible even without explicit visibility)
    TrackRegion region;
    region.startTime = 0.0;
    region.duration = 5.0;
    track.addRegion(region);

    EXPECT_TRUE(track.isVisible());  // Visible because it has regions
}

// Test Track height
TEST(TrackTest, Height) {
    Track track;
    EXPECT_EQ(track.getHeight(), 100);

    track.setHeight(200);
    EXPECT_EQ(track.getHeight(), 200);

    // Test minimum height constraint
    track.setHeight(30);
    EXPECT_EQ(track.getHeight(), 60);  // Should be clamped to minimum of 60
}

// Test Track color
TEST(TrackTest, Color) {
    Track track;
    EXPECT_EQ(track.getColor(), 0xFF4A90D9);

    track.setColor(0xFF00FF00);  // Green
    EXPECT_EQ(track.getColor(), 0xFF00FF00);
}

// Test Track regions
TEST(TrackTest, Regions) {
    Track track;
    EXPECT_TRUE(track.getRegions().empty());

    TrackRegion region1;
    region1.startTime = 0.0;
    region1.duration = 5.0;

    TrackRegion region2;
    region2.startTime = 10.0;
    region2.duration = 3.0;

    track.addRegion(region1);
    EXPECT_EQ(track.getRegions().size(), 1);

    track.addRegion(region2);
    EXPECT_EQ(track.getRegions().size(), 2);

    track.removeRegion(0);
    EXPECT_EQ(track.getRegions().size(), 1);
    EXPECT_DOUBLE_EQ(track.getRegions()[0].startTime, 10.0);
}
