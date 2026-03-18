#pragma once

#include <Arduino.h>
#include <Preferences.h>

//=============================================================================
// SettingsManager - Persistent Settings Storage using ESP32 NVS
// HEADER-ONLY VERSION (No .cpp file needed!)
//=============================================================================

class SettingsManager
{
private:
    Preferences preferences;

    // Namespace untuk storage
    static constexpr const char *NAMESPACE = "console";

    // Keys untuk setiap setting
    static constexpr const char *KEY_BRIGHTNESS = "brightness";
    static constexpr const char *KEY_DIFFICULTY = "difficulty";
    static constexpr const char *KEY_VOLUME = "volume";
    static constexpr const char *KEY_SNAKE_HIGH = "snake_high";
    static constexpr const char *KEY_PONG_HIGH = "pong_high";
    static constexpr const char *KEY_RACING_HIGH = "racing_high";
    static constexpr const char *KEY_DINO_HIGH = "dino_high";
    static constexpr const char *KEY_GD_HIGH = "gd_high";

    // Default values
    static constexpr int DEFAULT_BRIGHTNESS = 1; // Medium
    static constexpr int DEFAULT_DIFFICULTY = 0; // Easy
    static constexpr int DEFAULT_VOLUME = 2;     // Medium

public:
    //=========================================================================
    // INITIALIZATION
    //=========================================================================

    bool begin()
    {
        bool success = preferences.begin(NAMESPACE, false);

        if (success)
        {
            Serial.println("[SettingsMgr] NVS initialized");

            if (!preferences.isKey(KEY_BRIGHTNESS))
            {
                Serial.println("[SettingsMgr] First boot - loading defaults");
                loadDefaults();
            }
            else
            {
                Serial.println("[SettingsMgr] Settings loaded from NVS");
            }
        }
        else
        {
            Serial.println("[SettingsMgr] ERROR: Failed to initialize NVS!");
        }

        return success;
    }

    void end()
    {
        preferences.end();
    }

    //=========================================================================
    // BRIGHTNESS (0-4: Off, Low, Med, High, Max)
    //=========================================================================

    void setBrightness(int value)
    {
        value = constrain(value, 0, 4);
        preferences.putInt(KEY_BRIGHTNESS, value);
        Serial.printf("[SettingsMgr] Brightness set to %d\n", value);
    }

    int getBrightness()
    {
        return preferences.getInt(KEY_BRIGHTNESS, DEFAULT_BRIGHTNESS);
    }

    //=========================================================================
    // DIFFICULTY (0-2: Easy, Medium, Hard)
    //=========================================================================

    void setDifficulty(int value)
    {
        value = constrain(value, 0, 2);
        preferences.putInt(KEY_DIFFICULTY, value);
        Serial.printf("[SettingsMgr] Difficulty set to %d\n", value);
    }

    int getDifficulty()
    {
        return preferences.getInt(KEY_DIFFICULTY, DEFAULT_DIFFICULTY);
    }

    //=========================================================================
    // VOLUME (0-4: Mute, Low, Med, High, Max)
    //=========================================================================

    void setVolume(int value)
    {
        value = constrain(value, 0, 4);
        preferences.putInt(KEY_VOLUME, value);
        Serial.printf("[SettingsMgr] Volume set to %d\n", value);
    }

    int getVolume()
    {
        return preferences.getInt(KEY_VOLUME, DEFAULT_VOLUME);
    }

    //=========================================================================
    // HIGH SCORES
    //=========================================================================

    void setSnakeHighScore(unsigned long score)
    {
        preferences.putULong(KEY_SNAKE_HIGH, score);
        Serial.printf("[SettingsMgr] Snake high score: %lu\n", score);
    }

    unsigned long getSnakeHighScore()
    {
        return preferences.getULong(KEY_SNAKE_HIGH, 0);
    }

    void setPongHighScore(unsigned long score)
    {
        preferences.putULong(KEY_PONG_HIGH, score);
        Serial.printf("[SettingsMgr] Pong high score: %lu\n", score);
    }

    unsigned long getPongHighScore()
    {
        return preferences.getULong(KEY_PONG_HIGH, 0);
    }

    void setRacingHighScore(unsigned long score)
    {
        preferences.putULong(KEY_RACING_HIGH, score);
        Serial.printf("[SettingsMgr] Racing high score: %lu\n", score);
    }

    unsigned long getRacingHighScore()
    {
        return preferences.getULong(KEY_RACING_HIGH, 0);
    }

    void setDinoHighScore(unsigned long score)
    {
        preferences.putULong(KEY_DINO_HIGH, score);
        Serial.printf("[SettingsMgr] Dino high score: %lu\n", score);
    }

    unsigned long getDinoHighScore()
    {
        return preferences.getULong(KEY_DINO_HIGH, 0);
    }

    void setGeoDashHighScore(unsigned long score)
    {
        preferences.putULong(KEY_GD_HIGH, score);
        Serial.printf("[SettingsMgr] GeoDash high score: %lu\n", score);
    }

    unsigned long getGeoDashHighScore()
    {
        return preferences.getULong(KEY_GD_HIGH, 0);
    }

    //=========================================================================
    // BULK OPERATIONS
    //=========================================================================

    void loadDefaults()
    {
        setBrightness(DEFAULT_BRIGHTNESS);
        setDifficulty(DEFAULT_DIFFICULTY);
        setVolume(DEFAULT_VOLUME);

        setSnakeHighScore(0);
        setPongHighScore(0);
        setRacingHighScore(0);
        setDinoHighScore(0);
        setGeoDashHighScore(0);

        Serial.println("[SettingsMgr] Defaults loaded");
    }

    void resetToDefaults()
    {
        Serial.println("[SettingsMgr] Resetting to defaults...");
        loadDefaults();
    }

    void clearAllData()
    {
        Serial.println("[SettingsMgr] Clearing all NVS data...");
        preferences.clear();
        loadDefaults();
        Serial.println("[SettingsMgr] All data cleared");
    }

    //=========================================================================
    // DEBUG / INFO
    //=========================================================================

    void printSettings()
    {
        Serial.println("\n=== Current Settings ===");
        Serial.printf("Brightness: %d\n", getBrightness());
        Serial.printf("Difficulty: %d\n", getDifficulty());
        Serial.printf("Volume: %d\n", getVolume());
        Serial.println("\n=== High Scores ===");
        Serial.printf("Snake: %lu\n", getSnakeHighScore());
        Serial.printf("Pong: %lu\n", getPongHighScore());
        Serial.printf("Racing: %lu\n", getRacingHighScore());
        Serial.printf("Dino: %lu\n", getDinoHighScore());
        Serial.printf("GeoDash: %lu\n", getGeoDashHighScore());
        Serial.println("====================\n");
    }

    void printSettingsSummary()
    {
        Serial.println("Brightness: " + String(getBrightness()) + " | Difficulty: " + String(getDifficulty()) + " | Volume: " + String(getVolume()));
    }

    size_t getFreeEntries()
    {
        return preferences.freeEntries();
    }
};