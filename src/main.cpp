#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <IRremote.hpp>

// ==================== INISIALISASI ====================
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
Adafruit_MPU6050 mpu;

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
const int IR_RECEIVE_PIN = 26;
const int IR_SEND_PIN = 17;
uint32_t savedCode = 0;
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
const unsigned long WIFI_SCAN_INTERVAL = 10000;  // interval antar scan (ms)
const unsigned long WIFI_SCAN_TIMEOUT_MS = 5000; // timeout maksimal nunggu hasil

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

// ==================== GEOMETRY DASH VARIABLES ====================
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

// =========== Bitmap Animator (sistem modular baru) =============
#include "Bitmap/Bitmap_Config.h"
#include "Bitmap/Bitmap_anim.h"

BitmapAnim StartupAnim;
BitmapAnim WifiScanAnim;

// ==================== FUNCTION DECLARATIONS ====================
void drawMenu();
void handleButtonPress();
void launchGame(int gameIndex);
void exitGame();

void initCarObstacleGame();
void updateCarObstacleGame();
void drawCarObstacleGame();

void initPongGame();
void updatePongGame();
void drawPongGame();

void initSnakeGame();
void updateSnakeGame();
void drawSnakeGame();

void updateAngleMonitor();
void drawAngleMonitor();

void updateWifiScanner();
void drawWifiScanner();

void initDinoGame();
void updateDinoGame();
void drawDinoGame();

void initGeoDashGame();
void updateGeoDashGame();
void drawGeoDashGame();

void IRClonning();
void launchSelectedIrMenu();

void showCredit();

void showSettings();
void launchSelectedSetting();
void applySettings();

int FindCenterX(int Objwidth, int Width = SCREEN_WIDTH)
{
  return (Width - Objwidth) / 2;
}

// ==================== BUZZER ENGINE ====================
enum BuzzerState
{
  BUZZER_IDLE,
  BUZZER_PLAYING,
  BUZZER_WAITING
};

struct BuzzerEngine
{
  int pin;
  const int *melody;
  const int *duration;
  int length, currentNote, cachedVolume;
  int singleMelody[1], singleDuration[1];
  unsigned long lastTime;
  BuzzerState state;

  void init(int p);
  void play(const int *m, const int *d, int len);
  void update();
  void playOnceTone(int freq, int dur);
  bool isPlaying();
};

void BuzzerEngine::playOnceTone(int freq, int dur)
{
  singleMelody[0] = freq;
  singleDuration[0] = dur;
  play(singleMelody, singleDuration, 1);
}
void BuzzerEngine::init(int p)
{
  pin = p;
  state = BUZZER_IDLE;
}
void BuzzerEngine::play(const int *m, const int *d, int len)
{
  melody = m;
  duration = d;
  length = len;
  currentNote = 0;
  state = BUZZER_PLAYING;
  lastTime = millis();
  int volumePercent = VolumeLevelValues[currentSettings.volumeIndex];
  cachedVolume = map(volumePercent, 0, 100, 0, 255);
  if (cachedVolume == 0)
  {
    state = BUZZER_IDLE;
    return;
  }
  ledcWriteTone(Buzzer_Channel, melody[currentNote]);
  ledcWrite(Buzzer_Channel, cachedVolume);
}
void BuzzerEngine::update()
{
  if (state == BUZZER_IDLE)
    return;
  unsigned long now = millis();
  if (state == BUZZER_PLAYING)
  {
    if (now - lastTime >= (unsigned long)duration[currentNote])
    {
      ledcWriteTone(Buzzer_Channel, 0);
      lastTime = now;
      state = BUZZER_WAITING;
    }
  }
  else if (state == BUZZER_WAITING)
  {
    if (now - lastTime >= 20)
    {
      currentNote++;
      if (currentNote >= length)
      {
        ledcWriteTone(Buzzer_Channel, 0);
        state = BUZZER_IDLE;
      }
      else
      {
        if (melody[currentNote] != 0)
        {
          ledcWriteTone(Buzzer_Channel, melody[currentNote]);
          ledcWrite(Buzzer_Channel, cachedVolume);
        }
        lastTime = now;
        state = BUZZER_PLAYING;
      }
    }
  }
}
bool BuzzerEngine::isPlaying() { return state != BUZZER_IDLE; }

#include <BuzzerNote.h>

const int melody_Startup[] = {NOTE_E5, NOTE_G5, NOTE_C6, NOTE_E6};
const int duration_Startup[] = {120, 120, 180, 300};

BuzzerEngine buzzer;

// ==================== SETUP ====================
void setup()
{
  Wire.begin(21, 22);
  Serial.begin(921600);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(Button, INPUT_PULLUP);

  u8g2.begin();
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_7x14_mf);
  u8g2.setColorIndex(1);

  ledcSetup(0, 2000, 8);
  ledcAttachPin(BUZZER_PIN, 0);

  if (!mpu.begin())
  {
    Serial.println("MPU6050 not detected!");
    while (1)
      delay(100);
  }

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  mpuOffsetX = a.acceleration.x;
  mpuOffsetY = a.acceleration.y;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init anim
  StartupAnim.init((const uint8_t *)Startup_frames, 48, 48, 28, 21, 40, 8, ANIM_LOOP, ANIM_DIR_H, true, ANIM_MSB);
  WifiScanAnim.init((const uint8_t *)WifiScanning_frames, 32, 32, 28, 21, 48, 8, ANIM_LOOP, ANIM_DIR_H, true, ANIM_MSB);

  currentSettings.brightnessIndex = brightnessLevel;
  currentSettings.difficultyIndex = difficultyLevel;
  currentSettings.volumeIndex = volumeLevel;
  applySettings();

  buzzer.init(BUZZER_PIN);
  buzzer.play(melody_Startup, duration_Startup, sizeof(melody_Startup) / sizeof(melody_Startup[0]));
  Serial.println("Gaming Console Started!");
}

// Startup Variables
unsigned long startupStartTime = 2000;

// ==================== MAIN LOOP ====================
void loop()
{

  buzzer.update();

  if (millis() < startupStartTime)
  {

    int intervaldot = millis() / 500 % 4;
    u8g2.setFont(u8g2_font_5x8_tf);
    char scanMsg[20];
    sprintf(scanMsg, "Starting Console%s", intervaldot == 0 ? "" : intervaldot == 1 ? "."
                                                               : intervaldot == 2   ? ".."
                                                                                    : "...");

    StartupAnim.update();
    u8g2.clearBuffer();
    StartupAnim.draw(u8g2); // draw() gantikan render()
    u8g2.drawStr(20, 61, scanMsg);
    u8g2.sendBuffer();
    return;
  }

  if (!inGame)
  {
    handleButtonPress();
    drawMenu();
  }
  else
  {
    switch (currentGame)
    {
    case 0:
      updatePongGame();
      drawPongGame();
      break;
    case 1:
      updateCarObstacleGame();
      drawCarObstacleGame();
      break;
    case 2:
      updateSnakeGame();
      drawSnakeGame();
      break;
    case 3:
      updateAngleMonitor();
      drawAngleMonitor();
      break;
    case 4:
      updateWifiScanner();
      drawWifiScanner();
      break;
    case 5:
      updateDinoGame();
      drawDinoGame();
      break;
    case 6:
      updateGeoDashGame();
      drawGeoDashGame();
      break;
    case 7:
      IRClonning();
      break;
    case 8:
      showCredit();
      break;
    case 9:
      showSettings();
      break;
    }

    int button_state = digitalRead(Button);
    if (button_state == LOW && !isPressed)
    {
      isPressed = true;
      startPressTime = millis();
    }
    if (button_state == HIGH && isPressed)
    {
      unsigned long dur = millis() - startPressTime;
      isPressed = false;
      if (dur >= longPressLimit)
      {
        if (inGameMenu)
        {
          if (currentGame == 9)
          {
            launchSelectedSetting();
          }
          else if (currentGame == 7)
          {
            launchSelectedIrMenu();
          }
          return;
        }
        exitGame();
        RunOnce = false;
      }
      else
      {
        ButtonShortPressedGame = true;
      }
    }
  }
}

// ==================== MENU FUNCTIONS ====================
void handleButtonPress()
{
  int button_state = digitalRead(Button);
  if (button_state == LOW && !isPressed)
  {
    isPressed = true;
    startPressTime = millis();
  }
  if (button_state == HIGH && isPressed)
  {
    isPressed = false;
    unsigned long dur = millis() - startPressTime;
    if (dur >= longPressLimit)
      launchGame(item_selected);
    else
      item_selected = (item_selected + 1) % NUM_ITEMS;
  }
}

void drawScrollbar()
{
  int scrollbarY = 3 + ((64 / NUM_ITEMS) * item_selected);
  u8g2.drawBox(125, scrollbarY, 3, 5);
}

void drawMenu()
{
  item_sel_previous = (item_selected - 1 + NUM_ITEMS) % NUM_ITEMS;
  item_sel_next = (item_selected + 1) % NUM_ITEMS;

  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_7x14_mf);
    u8g2.drawStr(26, 16, menu_item[item_sel_previous]);
    u8g2.drawBitmap(3, 2, 2, 16, bitmap_Icons[item_sel_previous]);

    u8g2.setFont(u8g2_font_7x14B_mf);
    u8g2.drawStr(26, 38, menu_item[item_selected]);
    u8g2.drawBitmap(3, 24, 2, 16, bitmap_Icons[item_selected]);

    u8g2.setFont(u8g2_font_7x14_mf);
    u8g2.drawStr(26, 58, menu_item[item_sel_next]);
    u8g2.drawBitmap(3, 46, 2, 16, bitmap_Icons[item_sel_next]);

    u8g2.drawBitmap(0, 22, 16, 21, epd_bitmap__Selected_outline);
    u8g2.drawBitmap(120, 0, 1, 64, epd_bitmap__Scrolbar);
    drawScrollbar();
  } while (u8g2.nextPage());
}

void launchGame(int gameIndex)
{
  inGame = true;
  currentGame = gameIndex;
  switch (gameIndex)
  {
  case 0:
    initPongGame();
    break;
  case 1:
    initCarObstacleGame();
    break;
  case 2:
    initSnakeGame();
    break;
  case 5:
    initDinoGame();
    break;
  case 6:
    initGeoDashGame();
    break;
  case 7:
    inGameMenu = true;

    break;
  case 9:
    inGameMenu = true;
    break;
  }
  buzzer.playOnceTone(1000, 100);
}

void exitGame()
{
  inGame = false;
  onSettingElement = false;
  inGameMenu = false;
  onIrMenu = false;
  currentGame = -1;
  SelectedSetting = 0;
  SETTING_Selected_outlineY = 0;
  // Reset WiFi scan state agar mulai bersih saat masuk lagi
  wifiScanState = WIFI_IDLE;
  lastWifiScan = 0;
  WiFi.scanDelete();
  buzzer.playOnceTone(400, 200);
}

// ==================== PING PONG GAME ====================
// Difficulty changes:
//   Easy  → paddle H=20, ball speed=2, AI reaction=1
//   Med   → paddle H=15, ball speed=3, AI reaction=2
//   Hard  → paddle H=10, ball speed=4, AI reaction=3
void initPongGame()
{
  int diff = getDiff();

  // Paddle height berkurang saat kesulitan naik
  pongPaddleHeight = 20 - (diff * 5); // 20 / 15 / 10

  // Kecepatan bola meningkat
  pongBallSpeed = 2 + diff; // 2 / 3 / 4

  paddleY = (SCREEN_HEIGHT - pongPaddleHeight) / 2;
  ballX = 64;
  ballY = 32;
  ballVelX = pongBallSpeed;
  ballVelY = pongBallSpeed;
  pongScore = 0;

  Serial.printf("[Pong] diff=%d  padH=%d  speed=%d\n", diff, pongPaddleHeight, pongBallSpeed);
}

void updatePongGame()
{
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float tilt = a.acceleration.x;
  paddleY = constrain(paddleY + (int)(tilt * 3), 0, SCREEN_HEIGHT - pongPaddleHeight);

  ballX += ballVelX;
  ballY += ballVelY;

  if (ballY <= 0 || ballY >= SCREEN_HEIGHT - BALL_SIZE)
  {
    ballVelY = -ballVelY;
    buzzer.playOnceTone(800, 50);
  }
  if (ballX >= SCREEN_WIDTH - BALL_SIZE)
  {
    ballVelX = -ballVelX;
    buzzer.playOnceTone(600, 50);
  }
  if (ballX <= PADDLE_OFFSET + PADDLE_WIDTH &&
      ballY + BALL_SIZE >= paddleY && ballY <= paddleY + pongPaddleHeight)
  {
    ballVelX = abs(ballVelX);
    pongScore++;
    buzzer.playOnceTone(1200, 50);
  }
  if (ballX < 0)
  {
    if (pongScore > pongHighScore)
      pongHighScore = pongScore;
    initPongGame();
    buzzer.playOnceTone(400, 300);
  }
  delay(20);
}

void drawPongGame()
{
  u8g2.clearBuffer();
  u8g2.drawBox(PADDLE_OFFSET, paddleY, PADDLE_WIDTH, pongPaddleHeight);
  u8g2.drawBox(ballX, ballY, BALL_SIZE, BALL_SIZE);
  u8g2.setFont(u8g2_font_7x14_mf);
  u8g2.setCursor(2, 10);
  u8g2.print("S:");
  u8g2.print(pongScore);
  if (pongHighScore > 0)
  {
    u8g2.setCursor(70, 10);
    u8g2.print("HS:");
    u8g2.print(pongHighScore);
  }
  u8g2.sendBuffer();
}

// ==================== RACING GAME ====================
// Difficulty changes:
//   Easy  → 3 obstacles, speed=3, obstacle W=10
//   Med   → 4 obstacles, speed=4, obstacle W=12
//   Hard  → 5 obstacles, speed=6, obstacle W=14
void initCarObstacleGame()
{
  int diff = getDiff();

  racingObstacleCount = 3 + diff;                      // 3 / 4 / 5
  racingObstacleSpeed = 3 + diff + (diff > 0 ? 1 : 0); // 3 / 5 / 6

  player.x = 64;
  player.y = CAR_Y;
  racingScore = 0;

  for (int i = 0; i < racingObstacleCount; i++)
  {
    obstacles[i].x = random(ROAD_LEFT, ROAD_RIGHT - OBS_WIDTH);
    obstacles[i].y = -OBS_HEIGHT - (i * MIN_OBSTACLE_SPACING);
    obstacles[i].active = true;
  }

  Serial.printf("[Car] diff=%d  count=%d  speed=%d\n", diff, racingObstacleCount, racingObstacleSpeed);
}

void updateCarObstacleGame()
{
  int obsW = 10 + (getDiff() * 2); // 10 / 12 / 14  — rintangan lebih lebar di Hard

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float tilt = a.acceleration.y;
  player.x = constrain(player.x + (int)(tilt * 5), ROAD_LEFT, ROAD_RIGHT - CAR_WIDTH);

  for (int i = 0; i < racingObstacleCount; i++)
  {
    if (!obstacles[i].active || obstacles[i].y > SCREEN_HEIGHT)
    {
      obstacles[i].x = random(ROAD_LEFT, ROAD_RIGHT + 20);
      obstacles[i].y = -OBS_HEIGHT;
      obstacles[i].active = true;
    }
    obstacles[i].y += racingObstacleSpeed;

    if (obstacles[i].active &&
        obstacles[i].y + OBS_HEIGHT >= player.y &&
        obstacles[i].y <= player.y + CAR_HEIGHT &&
        obstacles[i].x + obsW >= player.x &&
        obstacles[i].x <= player.x + CAR_WIDTH)
    {
      if (racingScore > racingHighScore)
        racingHighScore = racingScore;
      initCarObstacleGame();
      buzzer.playOnceTone(400, 300);
    }
  }
  racingScore++;
  delay(GAME_DELAY);
}

void drawCarObstacleGame()
{
  int obsW = 10 + (getDiff() * 2);
  u8g2.clearBuffer();
  u8g2.drawBitmap(player.x, player.y, 24 / 8, 25, bitmap__Car);
  for (int i = 0; i < racingObstacleCount; i++)
  {
    if (obstacles[i].active)
      u8g2.drawBox(obstacles[i].x, obstacles[i].y, obsW, OBS_HEIGHT);
  }
  u8g2.setFont(u8g2_font_7x14_mf);
  u8g2.setCursor(2, 10);
  u8g2.print("S:");
  u8g2.print(racingScore);
  if (racingHighScore > 0)
  {
    u8g2.setCursor(70, 10);
    u8g2.print("HS:");
    u8g2.print(racingHighScore);
  }
  u8g2.sendBuffer();
}

// ==================== NOKIA SNAKE GAME ====================
// Difficulty changes:
//   Easy  → speed=200ms, wrap walls (tembus)
//   Med   → speed=130ms, wrap walls
//   Hard  → speed=80ms,  mati jika tabrak dinding
void initSnakeGame()
{
  int diff = getDiff();

  // Kecepatan gerak ular
  SNAKE_SPEED = 200 - (diff * 60); // 200 / 140 / 80 ms

  snakeLength = 5;
  snakeDir = 0;
  snakeScore = 0;

  for (int i = 0; i < snakeLength; i++)
  {
    snake[i].x = 30 - (i * SNAKE_SIZE);
    snake[i].y = 30;
  }
  foodX = random(5, SCREEN_WIDTH - 10);
  foodY = random(15, SCREEN_HEIGHT - 5);
  lastSnakeMove = millis();

  Serial.printf("[Snake] diff=%d  speed=%dms\n", diff, SNAKE_SPEED);
}

void updateSnakeGame()
{
  if (millis() - lastSnakeMove < (unsigned long)SNAKE_SPEED)
    return;
  lastSnakeMove = millis();

  if (ButtonShortPressedGame)
  {
    ButtonShortPressedGame = false;
    snakeDir = (snakeDir + 1) % 4;
  }

  for (int i = snakeLength - 1; i > 0; i--)
    snake[i] = snake[i - 1];

  switch (snakeDir)
  {
  case 0:
    snake[0].x += SNAKE_SIZE;
    break;
  case 1:
    snake[0].y += SNAKE_SIZE;
    break;
  case 2:
    snake[0].x -= SNAKE_SIZE;
    break;
  case 3:
    snake[0].y -= SNAKE_SIZE;
    break;
  }

  bool died = false;
  if (getDiff() < 2)
  {
    // Easy & Med: wrap (tembus dinding)
    if (snake[0].x < 0)
      snake[0].x = SCREEN_WIDTH - SNAKE_SIZE;
    if (snake[0].x >= SCREEN_WIDTH)
      snake[0].x = 0;
    if (snake[0].y < 12)
      snake[0].y = SCREEN_HEIGHT - SNAKE_SIZE;
    if (snake[0].y >= SCREEN_HEIGHT)
      snake[0].y = 12;
  }
  else
  {
    // Hard: mati jika tabrak dinding
    if (snake[0].x < 0 || snake[0].x >= SCREEN_WIDTH ||
        snake[0].y < 12 || snake[0].y >= SCREEN_HEIGHT)
      died = true;
  }

  // Self collision (semua difficulty)
  for (int i = 1; i < snakeLength; i++)
  {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
    {
      died = true;
      break;
    }
  }

  if (died)
  {
    if (snakeScore > snakeHighScore)
      snakeHighScore = snakeScore;
    initSnakeGame();
    buzzer.playOnceTone(400, 200);
    return;
  }

  if (abs(snake[0].x - foodX) < SNAKE_SIZE && abs(snake[0].y - foodY) < SNAKE_SIZE)
  {
    if (snakeLength < MAX_SNAKE_LENGTH)
      snakeLength++;
    snakeScore++;
    foodX = random(5, SCREEN_WIDTH - 10);
    foodY = random(15, SCREEN_HEIGHT - 5);
    buzzer.playOnceTone(1200, 100);
  }
}

void drawSnakeGame()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_mf);
  u8g2.setCursor(2, 10);
  u8g2.print("S:");
  u8g2.print(snakeScore);
  if (snakeHighScore > 0)
  {
    u8g2.setCursor(70, 10);
    u8g2.print("HS:");
    u8g2.print(snakeHighScore);
  }
  u8g2.drawFrame(0, 12, SCREEN_WIDTH, SCREEN_HEIGHT - 12);
  for (int i = 0; i < snakeLength; i++)
    u8g2.drawBox(snake[i].x, snake[i].y, SNAKE_SIZE, SNAKE_SIZE);
  u8g2.drawBox(foodX, foodY, FOOD_SIZE, FOOD_SIZE);
  u8g2.sendBuffer();
}

// ==================== ANGLE MONITOR ====================
void updateAngleMonitor()
{
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  angleX = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;
  angleY = atan2(a.acceleration.x, a.acceleration.z) * 180.0 / PI;
  angleZ = atan2(a.acceleration.y, a.acceleration.x) * 180.0 / PI;
  delay(50);
}
void drawAngleMonitor()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_mf);
  u8g2.setCursor(5, 15);
  u8g2.print("Angle Monitor");
  u8g2.setFont(u8g2_font_7x14_mf);
  u8g2.setCursor(5, 30);
  u8g2.print("X: ");
  u8g2.print(angleX, 1);
  u8g2.print(" deg");
  u8g2.setCursor(5, 44);
  u8g2.print("Y: ");
  u8g2.print(angleY, 1);
  u8g2.print(" deg");
  u8g2.setCursor(5, 58);
  u8g2.print("Z: ");
  u8g2.print(angleZ, 1);
  u8g2.print(" deg");
  u8g2.sendBuffer();
}

// ==================== WIFI SCANNER (ASYNC) ====================
// WiFi.scanNetworks(true) = non-blocking, sehingga loop() tetap berjalan
// dan BuzzerEngine tidak terganggu selama scanning.
//
// Alur state:
//   WIFI_IDLE    → millis() melebihi interval → mulai scan async → WIFI_SCANNING
//   WIFI_SCANNING → cek WiFi.scanComplete() setiap loop → jika selesai → ambil hasil → WIFI_DONE
//   WIFI_DONE    → tampilkan hasil, tunggu interval berikutnya → WIFI_IDLE

void updateWifiScanner()
{
  unsigned long now = millis();

  switch (wifiScanState)
  {

  case WIFI_IDLE:
    // Tunggu interval, lalu mulai scan async
    if (now - lastWifiScan >= WIFI_SCAN_INTERVAL || lastWifiScan == 0)
    {
      WiFi.scanNetworks(true); // true = async, tidak memblokir loop
      scanStartTime = now;
      wifiScanState = WIFI_SCANNING;
      Serial.println("[WiFi] Scan started (async)");
    }
    break;

  case WIFI_SCANNING:
  {
    // Cek apakah scan sudah selesai
    int result = WiFi.scanComplete();

    if (result == WIFI_SCAN_RUNNING)
    {
      // Scan masih berjalan, tidak perlu delay — buzzer tetap jalan
      if (now - scanStartTime > WIFI_SCAN_TIMEOUT_MS)
      {
        // Timeout — batalkan dan coba lagi nanti
        WiFi.scanDelete();
        lastWifiScan = now;
        wifiScanState = WIFI_IDLE;
        Serial.println("[WiFi] Scan timeout, will retry");
      }
      break;
    }

    // Scan selesai (result = jumlah network atau error negatif)
    if (result >= 0)
    {
      wifiNetworkCount = min(result, 10);
      for (int i = 0; i < wifiNetworkCount; i++)
      {
        wifiSSID[i] = WiFi.SSID(i);
        wifiRSSI[i] = WiFi.RSSI(i);
      }
      WiFi.scanDelete(); // bebaskan memori hasil scan
      buzzer.playOnceTone(1000, 50);
      Serial.printf("[WiFi] Scan done: %d networks\n", wifiNetworkCount);
    }
    else
    {
      // Error (result == WIFI_SCAN_FAILED, dll)
      wifiNetworkCount = 0;
      WiFi.scanDelete();
      Serial.println("[WiFi] Scan failed");
    }

    lastWifiScan = now;
    wifiScanState = WIFI_DONE;
    break;
  }

  case WIFI_DONE:
    // Tampilkan hasil sampai interval berikutnya
    if (now - lastWifiScan >= WIFI_SCAN_INTERVAL)
    {
      wifiScanState = WIFI_IDLE;
    }
    break;
  }
  // Tidak ada delay() di sini agar buzzer tetap responsif
}

void drawWifiScanner()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_mf);

  switch (wifiScanState)
  {
  case WIFI_IDLE:
  case WIFI_SCANNING:
  {
    // Tampilkan animasi titik berdasarkan waktu (tanpa delay)

    WifiScanAnim.update();
    WifiScanAnim.draw(u8g2);

    unsigned long elapsed = millis() - scanStartTime;
    int dots = (elapsed / 400) % 4; // 0..3 titik bergantian
    char scanMsg[20];
    snprintf(scanMsg, sizeof(scanMsg), "Scanning%s",
             dots == 0 ? "." : dots == 1 ? ".."
                           : dots == 2   ? "..."
                                         : "....");
    u8g2.setFont(u8g2_font_5x8_tf);
    int cx = FindCenterX(u8g2.getStrWidth(scanMsg));
    u8g2.drawStr(cx, 54, scanMsg);
    break;
  }

  case WIFI_DONE:
    u8g2.setCursor(5, 12);
    u8g2.print("WiFi: ");
    u8g2.print(wifiNetworkCount);
    u8g2.print(" found");

    u8g2.setFont(u8g2_font_6x10_mf);
    {
      int yPos = 24;
      for (int i = 0; i < min(4, wifiNetworkCount); i++)
      {
        u8g2.setCursor(2, yPos);
        String name = wifiSSID[i];
        if (name.length() > 15)
          name = name.substring(0, 15);
        u8g2.print(name);
        u8g2.print(" ");
        u8g2.print(wifiRSSI[i]);
        u8g2.print("dB");
        yPos += 11;
      }
      if (wifiNetworkCount == 0)
      {
        u8g2.setCursor(5, 40);
        u8g2.print("No networks found");
      }
    }
    break;
  }

  u8g2.sendBuffer();
}

// ==================== DINO RUNNER GAME ====================
// Difficulty changes:
//   Easy  → 2 obstacles, speed=3, speed cap=6
//   Med   → 3 obstacles, speed=4, speed cap=8
//   Hard  → 4 obstacles, speed=5, speed cap=10 (+ no speed cooldown)
void initDinoGame()
{
  int diff = getDiff();

  dinoObstacleCount = 2 + diff; // 2 / 3 / 4
  dinoObstacleSpeed = 3 + diff; // 3 / 4 / 5 (kecepatan awal)
  gameSpeed = dinoObstacleSpeed;

  dinoY = GROUND_Y;
  dinoVelY = 0;
  isJumping = false;
  isDiving = false;
  dinoScore = 0;
  lastSpeedIncrease = millis();

  for (int i = 0; i < dinoObstacleCount; i++)
  {
    dinoObstacles[i].x = SCREEN_WIDTH + (i * 50);
    dinoObstacles[i].y = GROUND_Y;
    dinoObstacles[i].active = true;
  }

  Serial.printf("[Dino] diff=%d  count=%d  initSpeed=%d\n", diff, dinoObstacleCount, gameSpeed);
}

void updateDinoGame()
{
  int diff = getDiff();
  int maxSpeed = 6 + (diff * 2);                 // 6 / 8 / 10
  int speedInterval = (diff == 2) ? 3000 : 5000; // Hard: makin cepat setiap 3 detik

  int button_state = digitalRead(Button);
  static bool buttonWasPressed = false;

  if (button_state == LOW && !buttonWasPressed)
  {
    if (!isJumping)
    {
      dinoVelY = JUMP_STRENGTH;
      isJumping = true;
      isDiving = false;
      buzzer.playOnceTone(1000, 50);
    }
    else if (!isDiving)
    {
      dinoVelY = DIVE_SPEED;
      isDiving = true;
      buzzer.playOnceTone(600, 50);
    }
    buttonWasPressed = true;
  }
  if (button_state == HIGH)
    buttonWasPressed = false;

  dinoVelY = isDiving ? DIVE_SPEED : dinoVelY + GRAVITY;
  dinoY += dinoVelY;

  if (dinoY >= GROUND_Y)
  {
    dinoY = GROUND_Y;
    dinoVelY = 0;
    isJumping = false;
    isDiving = false;
  }

  for (int i = 0; i < dinoObstacleCount; i++)
  {
    dinoObstacles[i].x -= gameSpeed;
    if (dinoObstacles[i].x < -OBS_WIDTH)
    {
      dinoObstacles[i].x = SCREEN_WIDTH + random(20, 60);
      dinoObstacles[i].active = true;
      dinoScore++;
    }
    if (dinoObstacles[i].active &&
        dinoObstacles[i].x < DINO_X + DINO_WIDTH &&
        dinoObstacles[i].x + OBS_WIDTH > DINO_X &&
        dinoY + DINO_HEIGHT > dinoObstacles[i].y)
    {
      if (dinoScore > dinoHighScore)
        dinoHighScore = dinoScore;
      initDinoGame();
      buzzer.playOnceTone(400, 300);
      delay(300);
    }
  }

  if (millis() - lastSpeedIncrease > (unsigned long)speedInterval)
  {
    if (gameSpeed < maxSpeed)
      gameSpeed++;
    lastSpeedIncrease = millis();
  }
  delay(30);
}

void drawDinoGame()
{
  u8g2.clearBuffer();
  u8g2.drawHLine(0, GROUND_Y + DINO_HEIGHT + 1, SCREEN_WIDTH);
  u8g2.drawBitmap(DINO_X, dinoY, 1, 8, dino_bitmap);
  if (isDiving)
    u8g2.drawTriangle(DINO_X + 4, dinoY - 5, DINO_X + 2, dinoY - 2, DINO_X + 6, dinoY - 2);
  for (int i = 0; i < dinoObstacleCount; i++)
  {
    if (dinoObstacles[i].active && dinoObstacles[i].x < SCREEN_WIDTH)
      u8g2.drawBox(dinoObstacles[i].x, dinoObstacles[i].y, OBS_WIDTH, OBS_HEIGHT);
  }
  u8g2.setFont(u8g2_font_7x14_mf);
  u8g2.setCursor(2, 10);
  u8g2.print("S:");
  u8g2.print(dinoScore);
  if (dinoHighScore > 0)
  {
    u8g2.setCursor(70, 10);
    u8g2.print("HS:");
    u8g2.print(dinoHighScore);
  }
  u8g2.sendBuffer();
}

// ==================== GEOMETRY DASH GAME ====================
// Difficulty changes:
//   Easy  → pillar speed=2, min gap=30px
//   Med   → pillar speed=3, min gap=24px
//   Hard  → pillar speed=4, min gap=18px  (+ pillar lebih sering muncul)
void initGeoDashGame()
{
  int diff = getDiff();

  gdPillarSpeed = 2 + diff;   // 2 / 3 / 4
  gdGapMin = 30 - (diff * 6); // 30 / 24 / 18  → celah makin sempit

  cubeX = 30;
  cubeY = SCREEN_HEIGHT / 2;
  cubeVelY = 0;
  cubeIsFlying = false;
  gdScore = 0;

  int spacing = 60 - (diff * 10); // 60 / 50 / 40 → pilar lebih rapat di Hard
  for (int i = 0; i < MAX_GD_OBSTACLES; i++)
  {
    gdObstacles[i].x = SCREEN_WIDTH + (i * spacing);
    gdObstacles[i].gapY = random(gdGapMin + 5, SCREEN_HEIGHT - gdGapMin - 5);
    gdObstacles[i].gapHeight = random(gdGapMin, gdGapMin + 12);
    gdObstacles[i].active = true;
  }

  Serial.printf("[GeoDash] diff=%d  speed=%d  gapMin=%d  spacing=%d\n", diff, gdPillarSpeed, gdGapMin, spacing);
}

void updateGeoDashGame()
{
  int diff = getDiff();
  int spacing = 60 - (diff * 10);

  int button_state = digitalRead(Button);
  if (button_state == LOW)
  {
    if (!cubeIsFlying)
    {
      cubeVelY = GD_JUMP_STRENGTH;
      cubeIsFlying = true;
      buzzer.playOnceTone(1200, 30);
    }
  }
  else
  {
    cubeIsFlying = false;
  }

  cubeVelY += GD_GRAVITY;
  cubeY += cubeVelY;

  if (cubeY <= 0 || cubeY >= SCREEN_HEIGHT - CUBE_SIZE)
  {
    if (gdScore > gdHighScore)
      gdHighScore = gdScore;
    initGeoDashGame();
    buzzer.playOnceTone(400, 300);
    delay(300);
    return;
  }

  for (int i = 0; i < MAX_GD_OBSTACLES; i++)
  {
    gdObstacles[i].x -= gdPillarSpeed;

    if (gdObstacles[i].x < -PILLAR_WIDTH)
    {
      gdObstacles[i].x = SCREEN_WIDTH + random(10, spacing);
      gdObstacles[i].gapY = random(gdGapMin + 5, SCREEN_HEIGHT - gdGapMin - 5);
      gdObstacles[i].gapHeight = random(gdGapMin, gdGapMin + 12);
      gdObstacles[i].active = true;
      gdScore++;
      buzzer.playOnceTone(800, 50);
    }

    if (gdObstacles[i].active &&
        gdObstacles[i].x < cubeX + CUBE_SIZE &&
        gdObstacles[i].x + PILLAR_WIDTH > cubeX)
    {
      int gapTop = gdObstacles[i].gapY - (gdObstacles[i].gapHeight / 2);
      int gapBottom = gdObstacles[i].gapY + (gdObstacles[i].gapHeight / 2);
      if (cubeY < gapTop || cubeY + CUBE_SIZE > gapBottom)
      {
        if (gdScore > gdHighScore)
          gdHighScore = gdScore;
        initGeoDashGame();
        buzzer.playOnceTone(400, 300);
        delay(300);
      }
    }
  }
  delay(30);
}

void drawGeoDashGame()
{
  u8g2.clearBuffer();
  u8g2.drawBitmap(cubeX, cubeY, 1, 6, cube_bitmap);
  for (int i = 0; i < MAX_GD_OBSTACLES; i++)
  {
    if (gdObstacles[i].active && gdObstacles[i].x < SCREEN_WIDTH && gdObstacles[i].x > -PILLAR_WIDTH)
    {
      int gapTop = gdObstacles[i].gapY - (gdObstacles[i].gapHeight / 2);
      int gapBottom = gdObstacles[i].gapY + (gdObstacles[i].gapHeight / 2);
      u8g2.drawBox(gdObstacles[i].x, 0, PILLAR_WIDTH, gapTop);
      u8g2.drawBox(gdObstacles[i].x, gapBottom, PILLAR_WIDTH, SCREEN_HEIGHT - gapBottom);
    }
  }
  u8g2.setFont(u8g2_font_7x14_mf);
  u8g2.setCursor(2, 10);
  u8g2.print("S:");
  u8g2.print(gdScore);
  if (gdHighScore > 0)
  {
    u8g2.setCursor(70, 10);
    u8g2.print("HS:");
    u8g2.print(gdHighScore);
  }
  u8g2.sendBuffer();
}

// ==================== CREDIT ====================
void showCredit()
{
  if (!RunOnce)
  {
    u8g2.clearBuffer();
    RunOnce = true;
  }
  if (ButtonShortPressedGame)
  {
    ButtonShortPressedGame = false;
    currentCreditIndex = (currentCreditIndex + 1) % totalCredits;
    u8g2.clearBuffer();
  }
  int CenterName = FindCenterX(u8g2.getStrWidth(credits[currentCreditIndex].name));
  u8g2.setFont(u8g2_font_7x14B_mf);
  u8g2.drawStr(5, 15, credits[currentCreditIndex].role);
  u8g2.setFont(u8g2_font_7x14_mf);
  u8g2.drawStr(CenterName, 40, credits[currentCreditIndex].name);
  u8g2.drawBitmap(0, 1, 16, 21, epd_bitmap__Selected_outline);
  u8g2.sendBuffer();
}

// ==================== SETTINGS FUNCTIONS ====================
void applySettings()
{
  u8g2.setContrast(BrightnessLevelValues[currentSettings.brightnessIndex]);
  Serial.printf("Settings applied → Brightness:%d  Difficulty:%d  Volume:%d\n",
                BrightnessLevelValues[currentSettings.brightnessIndex],
                DifficultyLevelValues[currentSettings.difficultyIndex],
                VolumeLevelValues[currentSettings.volumeIndex]);
}

void DrawSettingElement(int elementIndex)
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_8x13B_tf);
  int barWidth;
  const char *valueLabel;
  int centerX_SettingName = FindCenterX(u8g2.getStrWidth(settingNames[elementIndex]), 128);

  switch (elementIndex)
  {
  case 0:
    u8g2.drawStr(centerX_SettingName, 14, settingNames[0]);
    barWidth = map(currentSettings.brightnessIndex, 0, 4, 0, 90);
    valueLabel = BrightnessLevelLabels[currentSettings.brightnessIndex];
    break;
  case 1:
    u8g2.drawStr(centerX_SettingName, 14, settingNames[1]);
    barWidth = map(currentSettings.difficultyIndex, 0, 2, 0, 90);
    valueLabel = DifficultyLevelLabels[currentSettings.difficultyIndex];
    break;
  default:
    u8g2.drawStr(centerX_SettingName, 14, settingNames[2]);
    barWidth = map(currentSettings.volumeIndex, 0, 4, 0, 90);
    valueLabel = VolumeLevelLabels[currentSettings.volumeIndex];
    break;
  }

  int centerXLabel = FindCenterX(u8g2.getStrWidth(valueLabel), 32) + 23;
  u8g2.drawFrame(17, 28, 94, 20);
  if (barWidth > 0)
    u8g2.drawBox(19, 30, barWidth, 16);
  u8g2.setBitmapMode(0);
  u8g2.drawFrame(21, 20, 32, 16);
  u8g2.setFont(u8g2_font_7x13_mf);
  u8g2.drawStr(centerXLabel, 32, valueLabel);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(0, 56, "Press: Change Value");
  u8g2.drawStr(0, 64, "Hold: Back to Menu");
  u8g2.sendBuffer();
}

void toggleBrightnessSetting()
{
  if (!ButtonShortPressedGame)
    return;
  ButtonShortPressedGame = false;
  currentSettings.brightnessIndex = (currentSettings.brightnessIndex + 1) % 5;
  applySettings();
  buzzer.playOnceTone(1000, 50);
}
void toggleDifficultySetting()
{
  if (!ButtonShortPressedGame)
    return;
  ButtonShortPressedGame = false;
  currentSettings.difficultyIndex = (currentSettings.difficultyIndex + 1) % 3;
  applySettings();
  buzzer.playOnceTone(1000, 50);
}
void toggleSoundSetting()
{
  if (!ButtonShortPressedGame)
    return;
  ButtonShortPressedGame = false;
  currentSettings.volumeIndex = (currentSettings.volumeIndex + 1) % 5;
  applySettings();
  if (VolumeLevelValues[currentSettings.volumeIndex] > 0)
    buzzer.playOnceTone(1000, 100);
}

void launchSelectedSetting()
{
  inGameMenu = false;
  onSettingElement = true;
  buzzer.playOnceTone(1200, 100);
}

void showSettings()
{
  if (onSettingElement)
  {
    DrawSettingElement(SelectedSetting);
    switch (SelectedSetting)
    {
    case 0:
      toggleBrightnessSetting();
      break;
    case 1:
      toggleDifficultySetting();
      break;
    case 2:
      toggleSoundSetting();
      break;
    }
    return;
  }

  if (ButtonShortPressedGame)
  {
    ButtonShortPressedGame = false;
    SelectedSetting = (SelectedSetting + 1) % NUM_SETTING_MENU;
    SETTING_Selected_outlineY = SelectedSetting * SETTING_ITEM_HEIGHT;
    buzzer.playOnceTone(800, 50);
  }

  u8g2.firstPage();
  do
  {
    u8g2.drawBitmap(0, SETTING_Selected_outlineY, 128 / 8, 21, second_menu_bitmap__Icon_selectedOutline);

    u8g2.setDrawColor(SelectedSetting == 0 ? 0 : 1);
    u8g2.setFont(SelectedSetting == 0 ? u8g2_font_7x14B_mf : u8g2_font_7x14_mf);
    u8g2.drawStr(27, 16, setting_menu_bitmap_allArray_names[0]);
    u8g2.setDrawColor(1);
    u8g2.drawBitmap(3, 2, 2, 16, setting_menu_bitmap_allArray[0]);

    u8g2.setDrawColor(SelectedSetting == 1 ? 0 : 1);
    u8g2.setFont(SelectedSetting == 1 ? u8g2_font_7x14B_mf : u8g2_font_7x14_mf);
    u8g2.drawStr(27, 38, setting_menu_bitmap_allArray_names[1]);
    u8g2.setDrawColor(1);
    u8g2.drawBitmap(3, 24, 2, 16, setting_menu_bitmap_allArray[1]);

    u8g2.setDrawColor(SelectedSetting == 2 ? 0 : 1);
    u8g2.setFont(SelectedSetting == 2 ? u8g2_font_7x14B_mf : u8g2_font_7x14_mf);
    u8g2.drawStr(27, 58, setting_menu_bitmap_allArray_names[2]);
    u8g2.setDrawColor(1);
    u8g2.drawBitmap(3, 46, 2, 16, setting_menu_bitmap_allArray[2]);
  } while (u8g2.nextPage());
}

void DrawIrMenuElement(int elementIndex)
{
  u8g2.clearBuffer();
  switch (elementIndex)
  {
  case 0:
  {
    u8g2.setFont(u8g2_font_8x13B_tf);
    int centerX_SettingName = FindCenterX(u8g2.getStrWidth(IR_MENU[0]), 128);
    u8g2.drawStr(centerX_SettingName, 14, IR_MENU[0]);
    break;
  }
  case 1:
  {
    u8g2.setFont(u8g2_font_8x13B_tf);
    int centerX_SettingName_1 = FindCenterX(u8g2.getStrWidth(IR_MENU[1]), 128);
    u8g2.drawStr(centerX_SettingName_1, 14, IR_MENU[1]);
    break;
  }
    }
  u8g2.sendBuffer();
}

void launchSelectedIrMenu()
{
  inGameMenu = false;
  onIrMenu = true;
  buzzer.playOnceTone(1200, 100);
}

void IRClonning()
{

  if (onIrMenu)
  {
    DrawIrMenuElement(SelectedIrMenu);

    switch (SelectedIrMenu)
    {
    case 0:
      // IR Cloning
      Serial.println("Launching IR Cloning...");
      // Tambahkan kode untuk memulai proses IR Cloning di sini
      break;
    case 1:
      // IR Remote Control
      Serial.println("Launching IR Remote Control...");
      // Tambahkan kode untuk memulai proses IR Remote Control di sini
      break;
    default:
      break;
    }

    return;
  }

  if (ButtonShortPressedGame)
  {
    ButtonShortPressedGame = false;
    SelectedIrMenu = (SelectedIrMenu + 1) % NUM_IR_MENU;
    IR_MENU_Selected_outlineY = SelectedIrMenu * IR_MENU_ITEM_HEIGHT;
    buzzer.playOnceTone(800, 50);
  }

  u8g2.firstPage();
  do
  {
    u8g2.drawBitmap(0, IR_MENU_Selected_outlineY, 128 / 8, 21, second_menu_bitmap__Icon_selectedOutline);

    u8g2.setDrawColor(SelectedIrMenu == 0 ? 0 : 1);
    u8g2.setFont(SelectedIrMenu == 0 ? u8g2_font_7x14B_mf : u8g2_font_7x14_mf);
    u8g2.drawStr(27, 16, IR_MENU[0]);
    u8g2.setDrawColor(1);
    u8g2.drawBitmap(3, 2, 2, 16, setting_menu_bitmap_allArray[0]);

    u8g2.setDrawColor(SelectedIrMenu == 1 ? 0 : 1);
    u8g2.setFont(SelectedIrMenu == 1 ? u8g2_font_7x14B_mf : u8g2_font_7x14_mf);
    u8g2.drawStr(27, 38, IR_MENU[1]);
    u8g2.setDrawColor(1);
    u8g2.drawBitmap(3, 24, 2, 16, setting_menu_bitmap_allArray[1]);

  } while (u8g2.nextPage());
}