#include <gtest/gtest.h>
#include "../Track.h"

// Test TrackRegion default construction
TEST(TrackRegionTest, DefaultConstructor) {
    TrackRegion region;
    EXPECT_EQ(region.clip, nullptr);
    EXPECT_DOUBLE_EQ(region.startTime, 0.0);
    EXPECT_DOUBLE_EQ(region.clipOffset, 0.0);
    EXPECT_DOUBLE_EQ(region.duration, 0.0);
}

// Test TrackRegion endTime calculation
TEST(TrackRegionTest, EndTime) {
    TrackRegion region;
    region.startTime = 2.5;
    region.duration = 3.0;
    EXPECT_DOUBLE_EQ(region.endTime(), 5.5);
}

// Test TrackRegion with different values
TEST(TrackRegionTest, CustomValues) {
    TrackRegion region;
    region.startTime = 10.0;
    region.clipOffset = 1.5;
    region.duration = 5.0;

    EXPECT_DOUBLE_EQ(region.startTime, 10.0);
    EXPECT_DOUBLE_EQ(region.clipOffset, 1.5);
    EXPECT_DOUBLE_EQ(region.duration, 5.0);
    EXPECT_DOUBLE_EQ(region.endTime(), 15.0);
}

// Test TrackRegion with zero duration
TEST(TrackRegionTest, ZeroDuration) {
    TrackRegion region;
    region.startTime = 5.0;
    region.duration = 0.0;
    EXPECT_DOUBLE_EQ(region.endTime(), 5.0);
}

// Test TrackRegion with clip offset
TEST(TrackRegionTest, ClipOffset) {
    TrackRegion region;
    region.startTime = 0.0;
    region.clipOffset = 2.0;
    region.duration = 4.0;

    EXPECT_DOUBLE_EQ(region.clipOffset, 2.0);
    EXPECT_DOUBLE_EQ(region.endTime(), 4.0);
}
