#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// ==================== GLOBAL VARIABLES ====================
int item_selected = 0;
int item_sel_previous;
int item_sel_next;
bool inGame = false;
int currentGame = -1;
bool inGameMenu = false;

const int Button = 14;
bool isPressed = false;
unsigned long startPressTime = 0;
const unsigned long longPressLimit = 800;

bool ButtonShortPressedGame = false;
bool RunOnce = false;

const int BUZZER_PIN = 32;
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int Buzzer_Channel = 0;

float mpuOffsetX = 0;
float mpuOffsetY = 0;

// ===================== SETTING VARIABLES =====================
int brightnessLevel = 1;
int difficultyLevel = 0;
int volumeLevel = 2;
int SelectedSetting = 0;
const int SETTING_ITEM_HEIGHT = 21;
int SETTING_Selected_outlineY = 0;
bool onSettingElement = false;

struct Settings
{
    int brightnessIndex;
    int difficultyIndex;
    int volumeIndex;
};
Settings currentSettings;

int VolumeLevelValues[5] = {0, 25, 50, 75, 100};
int BrightnessLevelValues[5] = {0, 64, 128, 192, 255};
int DifficultyLevelValues[3] = {0, 1, 2};

const char *VolumeLevelLabels[5] = {"Mute", "Low", "Med", "High", "Max"};
const char *BrightnessLevelLabels[5] = {"Off", "Low", "Med", "High", "Max"};
const char *DifficultyLevelLabels[3] = {"Easy", "Med", "Hard"};
const char *settingNames[3] = {"Brightness", "Difficulty", "Volume"};

//=================== IR Cloning VARIABLES ====================
// ===== PIN =====
#define IR_RECEIVE_PIN 26
#define IR_SEND_PIN 17
#define IR_CAPTURE_BUF_SIZE 1024

bool codeSaved = false;
bool captureMode = false;
bool onIrMenu = false;

const int NUM_IR_MENU = 2;

const char IR_MENU[NUM_IR_MENU][20] = {
    "Capture IR",
    "Send IR"};

int SelectedIrMenu = 0;
int IR_MENU_ITEM_HEIGHT = 21;
int IR_MENU_Selected_outlineY = 0;
unsigned long CaptureStartTime = 0;

// ==================== DIFFICULTY HELPER ====================
// Ambil index difficulty saat ini (0=Easy, 1=Med, 2=Hard)
inline int getDiff()
{
    return currentSettings.difficultyIndex; // 0, 1, 2
}

// ==================== RACING GAME VARIABLES ====================
struct Car
{
    int x, y;
};
struct Obstacle
{
    int x, y;
    bool active;
};

const int CAR_WIDTH = 18;
const int CAR_HEIGHT = 25;
const int CAR_Y = 37;
const int OBS_WIDTH = 10;
const int OBS_HEIGHT = 10;
const int ROAD_LEFT = 7;
const int ROAD_RIGHT = SCREEN_WIDTH - ROAD_LEFT;
const int GAME_DELAY = 100;
const int MAX_OBSTACLES_HARD = 5; // nilai max absolut
const int MIN_OBSTACLE_SPACING = 30;

Car player = {64, CAR_Y};
Obstacle obstacles[MAX_OBSTACLES_HARD];
int racingObstacleCount = 3; // diset saat init berdasarkan difficulty
int racingObstacleSpeed = 3; // diset saat init
unsigned long racingScore = 0;
unsigned long racingHighScore = 0;

// ==================== PING PONG GAME VARIABLES ====================
const int PADDLE_WIDTH = 2;
const int PADDLE_OFFSET = 5;
const int BALL_SIZE = 2;

int pongPaddleHeight = 20; // diset saat init
int paddleY = 26;
int ballX = 64;
int ballY = 32;
int ballVelX = 2;
int ballVelY = 2;
int pongBallSpeed = 2; // diset saat init
unsigned long pongScore = 0;
unsigned long pongHighScore = 0;

// ==================== SNAKE GAME VARIABLES =====================
const int SNAKE_SIZE = 3;
const int MAX_SNAKE_LENGTH = 100;
const int FOOD_SIZE = 3;

struct SnakeSegment
{
    int x, y;
};
SnakeSegment snake[MAX_SNAKE_LENGTH];
int snakeLength = 5;
int snakeDir = 0;
int foodX = 60, foodY = 30;
unsigned long snakeScore = 0;
unsigned long snakeHighScore = 0;
unsigned long lastSnakeMove = 0;
int SNAKE_SPEED = 150; // diset saat init

// ==================== ANGLE MONITOR VARIABLES ====================
float angleX = 0, angleY = 0, angleZ = 0;

// ==================== WIFI SCANNER VARIABLES ====================
// Menggunakan WiFi.scanNetworks(true) = async, agar buzzer tidak terganggu
enum WifiScanState
{
    WIFI_IDLE,
    WIFI_SCANNING,
    WIFI_DONE
};

int wifiNetworkCount = 0;
String wifiSSID[10];
int wifiRSSI[10];
unsigned long lastWifiScan = 0;
unsigned long scanStartTime = 0;
WifiScanState wifiScanState = WIFI_IDLE;
const unsigned long WIFI_SCAN_INTERVAL = 16000;  // interval antar scan (ms)
const unsigned long WIFI_SCAN_TIMEOUT_MS = 8000; // timeout maksimal nunggu hasil

// ==================== DINO RUNNER VARIABLES ====================
const int DINO_WIDTH = 8;
const int DINO_HEIGHT = 8;
const int GROUND_Y = SCREEN_HEIGHT - DINO_HEIGHT;
const int DINO_X = 20;

int dinoY = GROUND_Y;
int dinoVelY = 0;
const int JUMP_STRENGTH = -8;
const int GRAVITY = 1;
const int DIVE_SPEED = 10;

const int MAX_DINO_OBS_HARD = 5;
Obstacle dinoObstacles[MAX_DINO_OBS_HARD];
int dinoObstacleCount = 2; // diset saat init
int dinoObstacleSpeed = 3; // diset saat init
unsigned long dinoScore = 0;
unsigned long dinoHighScore = 0;
bool isJumping = false;
bool isDiving = false;
int gameSpeed = 3;
unsigned long lastSpeedIncrease = 0;

// ==================== GEO BIRD VARIABLES ====================
const int CUBE_SIZE = 6;
const int GD_GRAVITY = 1;
const int GD_JUMP_STRENGTH = -6;
const int PILLAR_WIDTH = 8;
const int MIN_GAP_HEIGHT = 20;

int cubeX = 30;
int cubeY = SCREEN_HEIGHT - CUBE_SIZE;
int cubeVelY = 0;
bool cubeIsFlying = false;
int gdPillarSpeed = 3; // diset saat init
int gdGapMin = 24;     // diset saat init

struct GDObstacle
{
    int x, gapY, gapHeight;
    bool active;
};
const int MAX_GD_OBSTACLES = 3;
GDObstacle gdObstacles[MAX_GD_OBSTACLES];
unsigned long gdScore = 0;
unsigned long gdHighScore = 0;

// ==================== Credit Screen Variables ====================
int currentCreditIndex = 0;
struct Credit
{
    const char *role;
    const char *name;
};
Credit credits[] = {
    {"Project Creator", "Akhdan Rafif O"},
    {"Circuit Design", "Akhdan Rafif O"},
    {"Game Developer", "Akhdan Rafif O"},
    {"UI Designer", "Akhdan Rafif O"},
    {"Tester", "Akhdan Rafif O"},
    {"Social Media", "@akhdn.v"},
    {"Special Thanks", "You, the player!"}};
int totalCredits = sizeof(credits) / sizeof(credits[0]);

#endif