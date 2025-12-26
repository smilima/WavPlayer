#include "gtest/gtest.h"
#include "../Track.h"
#include <cmath>

// Test Track creation and basic properties
TEST(TrackTests, CreateTrack) {
    Track track(L"Test Track");

    EXPECT_EQ(track.getName(), L"Test Track");
    EXPECT_FLOAT_EQ(track.getVolume(), 1.0f);
    EXPECT_FLOAT_EQ(track.getPan(), 0.0f);
    EXPECT_FALSE(track.isMuted());
    EXPECT_FALSE(track.isSolo());
    EXPECT_FALSE(track.isArmed());
    EXPECT_TRUE(track.isVisible());
}

// Test volume controls
TEST(TrackTests, VolumeControl) {
    Track track(L"Test Track");

    track.setVolume(0.5f);
    EXPECT_FLOAT_EQ(track.getVolume(), 0.5f);

    track.setVolume(0.0f);
    EXPECT_FLOAT_EQ(track.getVolume(), 0.0f);

    track.setVolume(1.0f);
    EXPECT_FLOAT_EQ(track.getVolume(), 1.0f);
}

// Test pan controls
TEST(TrackTests, PanControl) {
    Track track(L"Test Track");

    // Center
    EXPECT_FLOAT_EQ(track.getPan(), 0.0f);

    // Left
    track.setPan(-1.0f);
    EXPECT_FLOAT_EQ(track.getPan(), -1.0f);

    // Right
    track.setPan(1.0f);
    EXPECT_FLOAT_EQ(track.getPan(), 1.0f);

    // Partial left
    track.setPan(-0.5f);
    EXPECT_FLOAT_EQ(track.getPan(), -0.5f);
}

// Test mute/solo functionality
TEST(TrackTests, MuteSoloControl) {
    Track track(L"Test Track");

    // Test mute
    EXPECT_FALSE(track.isMuted());
    track.setMuted(true);
    EXPECT_TRUE(track.isMuted());
    track.setMuted(false);
    EXPECT_FALSE(track.isMuted());

    // Test solo
    EXPECT_FALSE(track.isSolo());
    track.setSolo(true);
    EXPECT_TRUE(track.isSolo());
    track.setSolo(false);
    EXPECT_FALSE(track.isSolo());
}

// Test EQ controls
TEST(TrackTests, EQControl) {
    Track track(L"Test Track");

    // Default EQ should be 0 dB (neutral)
    EXPECT_FLOAT_EQ(track.getEQLow(), 0.0f);
    EXPECT_FLOAT_EQ(track.getEQMid(), 0.0f);
    EXPECT_FLOAT_EQ(track.getEQHigh(), 0.0f);

    // Test setting EQ values
    track.setEQLow(6.0f);
    track.setEQMid(-3.0f);
    track.setEQHigh(12.0f);

    EXPECT_FLOAT_EQ(track.getEQLow(), 6.0f);
    EXPECT_FLOAT_EQ(track.getEQMid(), -3.0f);
    EXPECT_FLOAT_EQ(track.getEQHigh(), 12.0f);
}

// Test peak level tracking
TEST(TrackTests, PeakLevelTracking) {
    Track track(L"Test Track");

    // Initial peak level should be 0
    EXPECT_FLOAT_EQ(track.getPeakLevel(), 0.0f);

    // Update peak level
    track.updatePeakLevel(0.5f);
    EXPECT_FLOAT_EQ(track.getPeakLevel(), 0.5f);

    // Higher level should update
    track.updatePeakLevel(0.8f);
    EXPECT_FLOAT_EQ(track.getPeakLevel(), 0.8f);

    // Lower level with decay should be less than current
    track.updatePeakLevel(0.1f);
    EXPECT_GT(track.getPeakLevel(), 0.1f);  // Should have decayed value
    EXPECT_LT(track.getPeakLevel(), 0.8f);  // But less than previous peak
}

// Test color property
TEST(TrackTests, ColorProperty) {
    Track track(L"Test Track");

    track.setColor(0xFF4A90D9);  // Blue color
    EXPECT_EQ(track.getColor(), 0xFF4A90D9);

    track.setColor(0xFFFF0000);  // Red color
    EXPECT_EQ(track.getColor(), 0xFFFF0000);
}

// Test visibility
TEST(TrackTests, Visibility) {
    Track track(L"Test Track");

    EXPECT_TRUE(track.isVisible());

    track.setVisible(false);
    EXPECT_FALSE(track.isVisible());

    track.setVisible(true);
    EXPECT_TRUE(track.isVisible());
}

// Test track naming
TEST(TrackTests, TrackNaming) {
    Track track(L"Original Name");

    EXPECT_EQ(track.getName(), L"Original Name");

    track.setName(L"New Name");
    EXPECT_EQ(track.getName(), L"New Name");
}
