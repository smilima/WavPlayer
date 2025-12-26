#include "gtest/gtest.h"
#include "../Settings.h"
#include <filesystem>

// Test fixture for Settings tests
class SettingsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Each test gets a fresh Settings object
        settings = std::make_unique<Settings>();
    }

    void TearDown() override {
        settings.reset();
    }

    std::unique_ptr<Settings> settings;
};

// Test default window settings
TEST_F(SettingsTest, DefaultWindowSettings) {
    EXPECT_EQ(settings->getWindowX(), 100);
    EXPECT_EQ(settings->getWindowY(), 100);
    EXPECT_EQ(settings->getWindowWidth(), 1280);
    EXPECT_EQ(settings->getWindowHeight(), 720);
    EXPECT_FALSE(settings->getWindowMaximized());
}

// Test window position setters/getters
TEST_F(SettingsTest, WindowPositionSettersGetters) {
    settings->setWindowX(200);
    settings->setWindowY(150);
    settings->setWindowWidth(1920);
    settings->setWindowHeight(1080);
    settings->setWindowMaximized(true);

    EXPECT_EQ(settings->getWindowX(), 200);
    EXPECT_EQ(settings->getWindowY(), 150);
    EXPECT_EQ(settings->getWindowWidth(), 1920);
    EXPECT_EQ(settings->getWindowHeight(), 1080);
    EXPECT_TRUE(settings->getWindowMaximized());
}

// Test default mixer window settings
TEST_F(SettingsTest, DefaultMixerWindowSettings) {
    EXPECT_EQ(settings->getMixerWindowX(), 100);
    EXPECT_EQ(settings->getMixerWindowY(), 100);
    EXPECT_EQ(settings->getMixerWindowWidth(), 800);
    EXPECT_EQ(settings->getMixerWindowHeight(), 600);
    EXPECT_FALSE(settings->getMixerWindowVisible());
}

// Test mixer window setters/getters
TEST_F(SettingsTest, MixerWindowSettersGetters) {
    settings->setMixerWindowX(300);
    settings->setMixerWindowY(250);
    settings->setMixerWindowWidth(1024);
    settings->setMixerWindowHeight(768);
    settings->setMixerWindowVisible(true);

    EXPECT_EQ(settings->getMixerWindowX(), 300);
    EXPECT_EQ(settings->getMixerWindowY(), 250);
    EXPECT_EQ(settings->getMixerWindowWidth(), 1024);
    EXPECT_EQ(settings->getMixerWindowHeight(), 768);
    EXPECT_TRUE(settings->getMixerWindowVisible());
}

// Test default timeline settings
TEST_F(SettingsTest, DefaultTimelineSettings) {
    EXPECT_DOUBLE_EQ(settings->getPixelsPerSecond(), 100.0);
    EXPECT_TRUE(settings->getFollowPlayhead());
    EXPECT_TRUE(settings->getShowGrid());
    EXPECT_TRUE(settings->getSnapToGrid());
    EXPECT_DOUBLE_EQ(settings->getBPM(), 120.0);
}

// Test timeline settings setters/getters
TEST_F(SettingsTest, TimelineSettingsSettersGetters) {
    settings->setPixelsPerSecond(150.0);
    settings->setFollowPlayhead(false);
    settings->setShowGrid(false);
    settings->setSnapToGrid(false);
    settings->setBPM(140.0);

    EXPECT_DOUBLE_EQ(settings->getPixelsPerSecond(), 150.0);
    EXPECT_FALSE(settings->getFollowPlayhead());
    EXPECT_FALSE(settings->getShowGrid());
    EXPECT_FALSE(settings->getSnapToGrid());
    EXPECT_DOUBLE_EQ(settings->getBPM(), 140.0);
}

// Test last project path
TEST_F(SettingsTest, LastProjectPath) {
    EXPECT_TRUE(settings->getLastProjectPath().empty());

    settings->setLastProjectPath(L"C:\\Users\\Test\\Documents\\project.austd");
    EXPECT_EQ(settings->getLastProjectPath(), L"C:\\Users\\Test\\Documents\\project.austd");
}

// Test edge cases for numeric values
TEST_F(SettingsTest, EdgeCaseNumericValues) {
    // Test very large window dimensions
    settings->setWindowWidth(3840);
    settings->setWindowHeight(2160);
    EXPECT_EQ(settings->getWindowWidth(), 3840);
    EXPECT_EQ(settings->getWindowHeight(), 2160);

    // Test very small pixels per second
    settings->setPixelsPerSecond(10.0);
    EXPECT_DOUBLE_EQ(settings->getPixelsPerSecond(), 10.0);

    // Test very high BPM
    settings->setBPM(300.0);
    EXPECT_DOUBLE_EQ(settings->getBPM(), 300.0);
}
