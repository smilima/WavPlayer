#include "gtest/gtest.h"
#include "../MixerWindow.h"
#include <cmath>

// Test dB to linear conversion
TEST(AudioUtilsTests, DBToLinear) {
    // Test 0 dB = 1.0 (unity gain)
    EXPECT_FLOAT_EQ(MixerWindow::dbToLinear(0.0f), 1.0f);

    // Test +6 dB (approximately 2x gain)
    float result = MixerWindow::dbToLinear(6.0f);
    EXPECT_NEAR(result, 2.0f, 0.01f);

    // Test -6 dB (approximately 0.5x gain)
    result = MixerWindow::dbToLinear(-6.0f);
    EXPECT_NEAR(result, 0.5f, 0.01f);

    // Test -60 dB (minimum, should be 0)
    result = MixerWindow::dbToLinear(-60.0f);
    EXPECT_FLOAT_EQ(result, 0.0f);

    // Test below minimum (should clamp to 0)
    result = MixerWindow::dbToLinear(-100.0f);
    EXPECT_FLOAT_EQ(result, 0.0f);
}

// Test linear to dB conversion
TEST(AudioUtilsTests, LinearToDB) {
    // Test 1.0 = 0 dB (unity gain)
    EXPECT_FLOAT_EQ(MixerWindow::linearToDB(1.0f), 0.0f);

    // Test 2.0 = approximately +6 dB
    float result = MixerWindow::linearToDB(2.0f);
    EXPECT_NEAR(result, 6.0f, 0.01f);

    // Test 0.5 = approximately -6 dB
    result = MixerWindow::linearToDB(0.5f);
    EXPECT_NEAR(result, -6.0f, 0.01f);

    // Test 0.0 = minimum dB
    result = MixerWindow::linearToDB(0.0f);
    EXPECT_FLOAT_EQ(result, MixerWindow::MIN_DB);

    // Test very small value (should clamp to MIN_DB)
    result = MixerWindow::linearToDB(0.00001f);
    EXPECT_GE(result, MixerWindow::MIN_DB);
}

// Test round-trip conversion (dB -> linear -> dB)
TEST(AudioUtilsTests, RoundTripDBConversion) {
    float testValues[] = {0.0f, -6.0f, -12.0f, -20.0f, -40.0f, 3.0f, 6.0f};

    for (float dbValue : testValues) {
        float linear = MixerWindow::dbToLinear(dbValue);
        float backToDb = MixerWindow::linearToDB(linear);

        // Should be very close after round-trip
        EXPECT_NEAR(backToDb, dbValue, 0.01f) << "Round-trip failed for " << dbValue << " dB";
    }
}

// Test round-trip conversion (linear -> dB -> linear)
TEST(AudioUtilsTests, RoundTripLinearConversion) {
    float testValues[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f};

    for (float linearValue : testValues) {
        float db = MixerWindow::linearToDB(linearValue);
        float backToLinear = MixerWindow::dbToLinear(db);

        // Should be very close after round-trip
        EXPECT_NEAR(backToLinear, linearValue, 0.01f) << "Round-trip failed for " << linearValue;
    }
}

// Test dB range limits
TEST(AudioUtilsTests, DBRangeLimits) {
    // Test that conversions respect MIN_DB and MAX_DB
    float minLinear = MixerWindow::dbToLinear(MixerWindow::MIN_DB);
    EXPECT_FLOAT_EQ(minLinear, 0.0f);

    float maxLinear = MixerWindow::dbToLinear(MixerWindow::MAX_DB);
    EXPECT_GT(maxLinear, 1.0f);  // +6 dB should be > 1.0

    // Test clamping
    float belowMin = MixerWindow::linearToDB(-1.0f);  // Negative linear
    EXPECT_EQ(belowMin, MixerWindow::MIN_DB);
}

// Test precision of conversion at critical points
TEST(AudioUtilsTests, CriticalDBPoints) {
    // Test -3 dB point (approximately 0.707)
    float linear = MixerWindow::dbToLinear(-3.0f);
    EXPECT_NEAR(linear, 0.707f, 0.01f);

    // Test -10 dB point (approximately 0.316)
    linear = MixerWindow::dbToLinear(-10.0f);
    EXPECT_NEAR(linear, 0.316f, 0.01f);

    // Test -20 dB point (0.1)
    linear = MixerWindow::dbToLinear(-20.0f);
    EXPECT_NEAR(linear, 0.1f, 0.01f);
}

// Test mathematical properties
TEST(AudioUtilsTests, MathematicalProperties) {
    // Adding 6 dB should approximately double the linear value
    float base = MixerWindow::dbToLinear(0.0f);   // 1.0
    float plus6 = MixerWindow::dbToLinear(6.0f);
    EXPECT_NEAR(plus6 / base, 2.0f, 0.01f);

    // Subtracting 6 dB should approximately halve the linear value
    float minus6 = MixerWindow::dbToLinear(-6.0f);
    EXPECT_NEAR(base / minus6, 2.0f, 0.01f);
}
