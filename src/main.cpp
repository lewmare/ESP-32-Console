// ============================================================================
//  ESP32 GAMING CONSOLE - Main Application
//  A feature-rich handheld gaming device with multiple mini-games,
//  WiFi scanning, IR cloning, and sensor monitoring capabilities.
// ============================================================================

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>

// ============================================================================
//  Hardware & Peripheral Initialization
// ============================================================================

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE); // OLED Display
Adafruit_MPU6050 mpu;                                            // Motion Sensor

#include "Config.h"
#include "StorageManager.h"
#include "IR_Clone.h"

SettingsManager settingsMgr; // Persistent settings storage
IRManager irManager;         // IR capture and transmission

// ============================================================================
//  Animation Engine (Bitmap Animations)
// ============================================================================

#include "Bitmap/Bitmap_Config.h"
#include "Bitmap/Bitmap_anim.h"

BitmapAnim StartupAnim;   // Initial startup animation
BitmapAnim WifiScanAnim;  // WiFi scanning animation
BitmapAnim IRCaptureAnim; // IR capture waiting animation
BitmapAnim IRSendingAnim; // IR transmission animation

// ============================================================================
//  Audio Engine (Buzzer/Sound)
// ============================================================================

#include "Buzzer/BuzzerEngine.h"
#include "Buzzer/BuzzerMelody.h"

BuzzerEngine buzzer; // Audio feedback system

// ============================================================================
//  Function Declarations
// ============================================================================

#include "Declarations.h"

// ============================================================================
//  System Initialization
// ============================================================================

void setup()
{
  // Initialize I2C communication
  Wire.begin(21, 22);
  Serial.begin(921600);

  // Load persistent settings from EEPROM
  settingsMgr.begin();
  currentSettings.brightnessIndex = settingsMgr.getBrightness();
  currentSettings.difficultyIndex = settingsMgr.getDifficulty();
  currentSettings.volumeIndex = settingsMgr.getVolume();
  applySettings();

  // Configure button inputs
  pinMode(EnterButton, INPUT_PULLUP);
  pinMode(UPButton, INPUT_PULLUP);
  pinMode(DOWNButton, INPUT_PULLUP);

  // Initialize OLED display
  u8g2.begin();
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_7x14_mf);
  u8g2.setColorIndex(1);

  // Configure PWM for buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  ledcSetup(0, 2000, 8);
  ledcAttachPin(BUZZER_PIN, 0);

  // Initialize motion sensor (accelerometer/gyroscope)
  if (!mpu.begin())
  {
    Serial.println("[ERROR] MPU6050 not detected!");
    while (1)
      delay(100);
  }

  // Discard initial sensor reading
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);

  // Configure WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize animation engines
  StartupAnim.init((const uint8_t *)Startup_frames, 48, 48, Startup_frames_COUNT, 21, 40, 8, ANIM_LOOP, ANIM_DIR_H, true, ANIM_MSB);
  WifiScanAnim.init((const uint8_t *)WifiScanning_frames, 32, 32, WifiScanning_frames_COUNT, 21, 48, 8, ANIM_LOOP, ANIM_DIR_H, true, ANIM_MSB);
  IRCaptureAnim.init((const uint8_t *)IR_WAITINGCAPTURE_frames, 32, 32, IR_WAITINGCAPTURE_frames_COUNT, 21, 48, 8, ANIM_LOOP, ANIM_DIR_H, true, ANIM_MSB);
  IRSendingAnim.init((const uint8_t *)IR_SENDING_frames, 32, 32, IR_SENDING_frames_COUNT, 21, 48, 8, ANIM_LOOP, ANIM_DIR_H, true, ANIM_MSB);

  // Initialize peripherals
  buzzer.init(BUZZER_PIN);
  buzzer.play(melody_Startup, duration_Startup, sizeof(melody_Startup) / sizeof(melody_Startup[0]));
  irManager.begin(IR_RECEIVE_PIN, IR_SEND_PIN);

  Serial.println("[SYSTEM] Gaming Console Started!");
}

// ============================================================================
//  Main Application Loop
// ============================================================================

void loop()
{
  buzzer.update();

  // Display startup animation during initialization
  if (millis() < startupStartTime)
  {
    _DisplayStartupScreen();
    settingsMgr.printSettingsSummary();
    return;
  }

  // Main menu interaction
  if (!inGame)
  {
    handleButtonPress();
    drawMenu();
  }
  else
  {
    // Handle active game
    _UpdateActiveGame();
    _HandleGameInput();
  }
}

// Helper function: Display animated startup screen with progress dots
void _DisplayStartupScreen()
{
  int intervalDot = millis() / 500 % 4;
  u8g2.setFont(u8g2_font_5x8_tf);
  char scanMsg[20];
  sprintf(scanMsg, "Starting Console%s",
          intervalDot == 0 ? "" : intervalDot == 1 ? "."
                              : intervalDot == 2   ? ".."
                                                   : "...");

  StartupAnim.update();
  u8g2.clearBuffer();
  StartupAnim.draw(u8g2);
  u8g2.drawStr(20, 61, scanMsg);
  u8g2.sendBuffer();
}

// Helper function: Update and render the currently active game
void _UpdateActiveGame()
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
}

// Helper function: Handle button input during gameplay
void _HandleGameInput()
{
  int EnterButtonState = digitalRead(EnterButton);
  int UPButtonState = digitalRead(UPButton);
  int DOWNButtonState = digitalRead(DOWNButton);

  // Handle Enter button (select/confirm)
  if (EnterButtonState == LOW && !EnterPressed)
  {
    EnterPressed = true;
    startPressTime = millis();
  }
  if (EnterButtonState == HIGH && EnterPressed)
  {
    unsigned long pressDuration = millis() - startPressTime;
    EnterPressed = false;
    if (pressDuration >= longPressLimit)
    {
      // Long press: exit game or go back from submenu
      if (currentCondition == ON_ELEMENT)
      {
        ToSubMenu();
      }
      else
      {
        exitGame();
        RunOnce = false;
      }
    }
    else
    {
      // Short press: trigger game action
      EnterButtonShortPressedGame = true;
    }
  }

  // Handle Up button
  if (UPButtonState == LOW && !UP_Pressed)
  {
    UP_Pressed = true;
    UPButtonShortPressedGame = true;
  }
  else if (UPButtonState == HIGH && UP_Pressed)
  {
    UP_Pressed = false;
    UPButtonShortPressedGame = false;
  }

  // Handle Down button
  if (DOWNButtonState == LOW && !DOWN_Pressed)
  {
    DOWN_Pressed = true;
    DOWNButtonShortPressedGame = true;
  }
  else if (DOWNButtonState == HIGH && DOWN_Pressed)
  {
    DOWN_Pressed = false;
    DOWNButtonShortPressedGame = false;
  }
}

// ============================================================================
//  Menu System & Navigation
// ============================================================================

void handleButtonPress()
{
  int EnterButtonState = digitalRead(EnterButton);
  int UPButtonState = digitalRead(UPButton);
  int DOWNButtonState = digitalRead(DOWNButton);

  // Navigate menu: Up button moves selection up (wraps around)
  if (UPButtonState == LOW && !UP_Pressed)
  {
    UP_Pressed = true;
    item_selected = (item_selected - 1 + NUM_ITEMS) % NUM_ITEMS;
  }
  else if (UPButtonState == HIGH && UP_Pressed)
  {
    UP_Pressed = false;
  }

  // Navigate menu: Down button moves selection down (wraps around)
  if (DOWNButtonState == LOW && !DOWN_Pressed)
  {
    DOWN_Pressed = true;
    item_selected = (item_selected + 1) % NUM_ITEMS;
  }
  else if (DOWNButtonState == HIGH && DOWN_Pressed)
  {
    DOWN_Pressed = false;
  }

  // Confirm selection: Enter button launches chosen game/feature
  if (EnterButtonState == LOW && !EnterPressed)
  {
    EnterPressed = true;
  }
  if (EnterButtonState == HIGH && EnterPressed)
  {
    EnterPressed = false;
    launchGame(item_selected);
  }
}

void drawMenu()
{
  item_sel_previous = (item_selected - 1 + NUM_ITEMS) % NUM_ITEMS;
  item_sel_next = (item_selected + 1) % NUM_ITEMS;

  u8g2.firstPage();
  do
  {
    // Render previous item (dimmed)
    u8g2.setFont(u8g2_font_7x14_mf);
    u8g2.drawStr(26, 16, menu_item[item_sel_previous]);
    u8g2.drawBitmap(3, 2, 2, 16, bitmap_Icons[item_sel_previous]);

    // Render selected item (highlighted)
    u8g2.setFont(u8g2_font_7x14B_mf);
    u8g2.drawStr(26, 38, menu_item[item_selected]);
    u8g2.drawBitmap(3, 24, 2, 16, bitmap_Icons[item_selected]);

    // Render next item (dimmed)
    u8g2.setFont(u8g2_font_7x14_mf);
    u8g2.drawStr(26, 58, menu_item[item_sel_next]);
    u8g2.drawBitmap(3, 46, 2, 16, bitmap_Icons[item_sel_next]);

    // Draw selection outline and scrollbar
    u8g2.drawBitmap(0, 22, 16, 21, epd_bitmap__Selected_outline);
    u8g2.drawBitmap(120, 0, 1, 64, epd_bitmap__Scrolbar);
    drawScrollbar();
  } while (u8g2.nextPage());
}

void drawScrollbar()
{
  // int scrollbarY = 3 + ((64 / NUM_ITEMS) * item_selected);

  int scrollbarY = (NUM_ITEMS > 0) ? 3 + ((64 / NUM_ITEMS) * item_selected) : 3;

  u8g2.drawBox(125, scrollbarY, 3, 5);
}

void launchGame(int gameIndex)
{
  inGame = true;
  currentGame = gameIndex;

  // Initialize game-specific state
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
  case 9:
    ToSubMenu(); // Enter submenu for IR and Settings
    break;
  }
  buzzer.playOnceTone(1000, 100);
}

void exitGame()
{
  inGame = false;
  ToggleCondition(MAIN_MENU);
  currentGame = -1;
  SelectedSetting = 0;
  SETTING_Selected_outlineY = 0;

  // Reset WiFi scan state to avoid stale results
  wifiScanState = WIFI_IDLE;
  lastWifiScan = 0;
  WiFi.scanDelete();

  buzzer.play(melody_exitGame, duration_exitGame, sizeof(melody_exitGame) / sizeof(melody_exitGame[0]));
}

void ToSubMenu()
{
  ToggleCondition(SUB_MENU);
  buzzer.play(melody_exitGame, duration_exitGame, sizeof(melody_exitGame) / sizeof(melody_exitGame[0]));
}

// ============================================================================
//  PING PONG GAME
// ============================================================================
// Difficulty Progression:
//   Easy  → paddle height=20px, ball speed=2px/frame
//   Med   → paddle height=15px, ball speed=3px/frame
//   Hard  → paddle height=10px, ball speed=4px/frame

void initPongGame()
{
  int difficulty = getDiff();

  pongPaddleHeight = 20 - (difficulty * 5); // 20 / 15 / 10
  pongBallSpeed = 2 + difficulty;           // 2 / 3 / 4

  paddleY = (SCREEN_HEIGHT - pongPaddleHeight) / 2;
  ballX = 64;
  ballY = 32;
  ballVelX = pongBallSpeed;
  ballVelY = pongBallSpeed;
  pongScore = 0;

  Serial.printf("[PONG] Difficulty=%d | Paddle Height=%dpx | Speed=%dpx/frame\n",
                difficulty, pongPaddleHeight, pongBallSpeed);
}

void updatePongGame()
{
  // Get accelerometer data to control paddle position
  sensors_event_t accelerometer, gyroscope, temperature;
  mpu.getEvent(&accelerometer, &gyroscope, &temperature);

  float tiltAngle = accelerometer.acceleration.x;
  paddleY = constrain(paddleY + (int)(tiltAngle * 3), 0, SCREEN_HEIGHT - pongPaddleHeight);

  // Update ball position
  ballX += ballVelX;
  ballY += ballVelY;

  // Bounce off top/bottom walls
  if (ballY <= 0 || ballY >= SCREEN_HEIGHT - BALL_SIZE)
  {
    ballVelY = -ballVelY;
    buzzer.playOnceTone(800, 50);
  }

  // Bounce off right wall
  if (ballX >= SCREEN_WIDTH - BALL_SIZE)
  {
    ballVelX = -ballVelX;
    buzzer.playOnceTone(600, 50);
  }

  // Paddle collision detection
  if (ballX <= PADDLE_OFFSET + PADDLE_WIDTH &&
      ballY + BALL_SIZE >= paddleY && ballY <= paddleY + pongPaddleHeight)
  {
    ballVelX = abs(ballVelX);
    pongScore++;
    buzzer.playOnceTone(1200, 50);
  }

  // Game over: ball exits left side
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

// ============================================================================
//  RACING GAME (Car Obstacle Avoidance)
// ============================================================================
// Difficulty Progression:
//   Easy  → 3 obstacles, speed=3px/frame, obstacle width=10px
//   Med   → 4 obstacles, speed=5px/frame, obstacle width=12px
//   Hard  → 5 obstacles, speed=6px/frame, obstacle width=14px

void initCarObstacleGame()
{
  int difficulty = getDiff();

  racingObstacleCount = 3 + difficulty;                            // 3 / 4 / 5
  racingObstacleSpeed = 3 + difficulty + (difficulty > 0 ? 1 : 0); // 3 / 5 / 6

  player.x = 64;
  player.y = CAR_Y;
  racingScore = 0;

  for (int i = 0; i < racingObstacleCount; i++)
  {
    obstacles[i].x = random(ROAD_LEFT, ROAD_RIGHT - OBS_WIDTH);
    obstacles[i].y = -OBS_HEIGHT - (i * MIN_OBSTACLE_SPACING);
    obstacles[i].active = true;
  }

  Serial.printf("[RACING] Difficulty=%d | Obstacle Count=%d | Speed=%dpx/frame\n",
                difficulty, racingObstacleCount, racingObstacleSpeed);
}

void updateCarObstacleGame()
{
  int obstacleWidth = 10 + (getDiff() * 2); // 10 / 12 / 14 (varies by difficulty)

  // Get accelerometer data to control car lane
  sensors_event_t accelerometer, gyroscope, temperature;
  mpu.getEvent(&accelerometer, &gyroscope, &temperature);
  float tiltY = accelerometer.acceleration.y;
  player.x = constrain(player.x + (int)(tiltY * 5), ROAD_LEFT, ROAD_RIGHT - CAR_WIDTH);

  // Update obstacle positions and check collisions
  for (int i = 0; i < racingObstacleCount; i++)
  {
    // Recycle obstacles that have left the screen
    if (!obstacles[i].active || obstacles[i].y > SCREEN_HEIGHT)
    {
      obstacles[i].x = random(ROAD_LEFT, ROAD_RIGHT + 20);
      obstacles[i].y = -OBS_HEIGHT;
      obstacles[i].active = true;
    }

    obstacles[i].y += racingObstacleSpeed;

    // Check collision with player car
    if (obstacles[i].active &&
        obstacles[i].y + OBS_HEIGHT >= player.y &&
        obstacles[i].y <= player.y + CAR_HEIGHT &&
        obstacles[i].x + obstacleWidth >= player.x &&
        obstacles[i].x <= player.x + CAR_WIDTH)
    {
      // Game over: reset with new high score if applicable
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
  int obstacleWidth = 10 + (getDiff() * 2);
  u8g2.clearBuffer();

  // Draw player car
  u8g2.drawBitmap(player.x, player.y, 24 / 8, 25, bitmap__Car);

  // Draw obstacles
  for (int i = 0; i < racingObstacleCount; i++)
  {
    if (obstacles[i].active)
      u8g2.drawBox(obstacles[i].x, obstacles[i].y, obstacleWidth, OBS_HEIGHT);
  }

  // Draw UI: Score and High Score
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

// ============================================================================
//  NOKIA SNAKE GAME
// ============================================================================
// Difficulty Progression:
//   Easy  → speed=200ms, wrap walls (infinite play)
//   Med   → speed=130ms, wrap walls (infinite play)
//   Hard  → speed=80ms, solid walls (game over on wall collision)

void initSnakeGame()
{
  int difficulty = getDiff();

  SNAKE_SPEED = 200 - (difficulty * 60); // 200 / 140 / 80 ms

  snakeLength = 5;
  snakeDir = 0;
  snakeScore = 0;

  // Initialize snake in center of play area
  for (int i = 0; i < snakeLength; i++)
  {
    snake[i].x = 30 - (i * SNAKE_SIZE);
    snake[i].y = 30;
  }

  foodX = random(5, SCREEN_WIDTH - 10);
  foodY = random(15, SCREEN_HEIGHT - 5);
  lastSnakeMove = millis();

  Serial.printf("[SNAKE] Difficulty=%d | Speed=%dms\n", difficulty, SNAKE_SPEED);
}

void updateSnakeGame()
{
  // Handle direction change when Up button is pressed
  if (UPButtonShortPressedGame)
  {
    UPButtonShortPressedGame = false;
    snakeDir = (snakeDir + 1) % 4; // Cycle through directions: 0→1→2→3→0
  }

  // Limit movement speed based on difficulty
  if (millis() - lastSnakeMove < (unsigned long)SNAKE_SPEED)
    return;

  lastSnakeMove = millis();

  // Shift snake body: move each segment to the position of the previous segment
  for (int i = snakeLength - 1; i > 0; i--)
    snake[i] = snake[i - 1];

  // Move snake head in current direction
  switch (snakeDir)
  {
  case 0: // Right
    snake[0].x += SNAKE_SIZE;
    break;
  case 1: // Down
    snake[0].y += SNAKE_SIZE;
    break;
  case 2: // Left
    snake[0].x -= SNAKE_SIZE;
    break;
  case 3: // Up
    snake[0].y -= SNAKE_SIZE;
    break;
  }

  bool died = false;

  // Handle wall collisions based on difficulty
  if (getDiff() < 2)
  {
    // Easy & Med: Walls are wrapped (snake goes through to other side)
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
    // Hard: Walls are solid (collision = death)
    if (snake[0].x < 0 || snake[0].x >= SCREEN_WIDTH ||
        snake[0].y < 12 || snake[0].y >= SCREEN_HEIGHT)
      died = true;
  }

  // Check self-collision: head hits body
  for (int i = 1; i < snakeLength; i++)
  {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
    {
      died = true;
      break;
    }
  }

  // Game over: reset game with high score tracking
  if (died)
  {
    if (snakeScore > snakeHighScore)
      snakeHighScore = snakeScore;
    initSnakeGame();
    buzzer.playOnceTone(400, 200);
    return;
  }

  // Food collision: snake ate food
  if (abs(snake[0].x - foodX) < SNAKE_SIZE && abs(snake[0].y - foodY) < SNAKE_SIZE)
  {
    // Grow snake if not at maximum length
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

  // Draw play area border
  u8g2.drawFrame(0, 12, SCREEN_WIDTH, SCREEN_HEIGHT - 12);

  // Draw snake body
  for (int i = 0; i < snakeLength; i++)
    u8g2.drawBox(snake[i].x, snake[i].y, SNAKE_SIZE, SNAKE_SIZE);

  // Draw food
  u8g2.drawBox(foodX, foodY, FOOD_SIZE, FOOD_SIZE);

  // Draw UI: Score and High Score
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

  u8g2.sendBuffer();
}

// ============================================================================
//  ACCELEROMETER ANGLE MONITOR
// ============================================================================
// Real-time display of device tilt angles calculated from accelerometer data

void updateAngleMonitor()
{
  sensors_event_t accelerometer, gyroscope, temperature;
  mpu.getEvent(&accelerometer, &gyroscope, &temperature);

  // Calculate angles from acceleration vectors
  angleX = atan2(accelerometer.acceleration.y, accelerometer.acceleration.z) * 180.0 / PI;
  angleY = atan2(accelerometer.acceleration.x, accelerometer.acceleration.z) * 180.0 / PI;

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
  u8g2.print(angleX, 1); // Print with 1 decimal place
  u8g2.print(" deg");

  u8g2.setCursor(5, 44);
  u8g2.print("Y: ");
  u8g2.print(angleY, 1); // Print with 1 decimal place
  u8g2.print(" deg");

  u8g2.sendBuffer();
}

// ============================================================================
//  WiFi NETWORK SCANNER
// ============================================================================
// Asynchronous scanning that doesn't block the main loop or audio engine.
// Uses a state machine for scan lifecycle management:
//   WIFI_IDLE    → Wait for scan interval → Start async scan → WIFI_SCANNING
//   WIFI_SCANNING → Poll for completion → Process results → WIFI_DONE
//   WIFI_DONE    → Display results → Wait for interval → Reset to WIFI_IDLE

void updateWifiScanner()
{
  unsigned long now = millis();

  switch (wifiScanState)
  {
  case WIFI_IDLE:
    // Start new scan if enough time has passed since last scan
    if (now - lastWifiScan >= WIFI_SCAN_INTERVAL || lastWifiScan == 0)
    {
      WiFi.scanNetworks(true); // true = async (non-blocking)
      scanStartTime = now;
      wifiScanState = WIFI_SCANNING;
      Serial.println("[WiFi] Scan started (async)");
    }
    break;

  case WIFI_SCANNING:
  {
    // Poll for scan completion without blocking
    int result = WiFi.scanComplete();

    if (result == WIFI_SCAN_RUNNING)
    {
      // Still scanning... check for timeout to avoid hanging
      if (now - scanStartTime > WIFI_SCAN_TIMEOUT_MS)
      {
        // Timeout: force cleanup and retry
        WiFi.scanDelete();
        lastWifiScan = now;
        wifiScanState = WIFI_IDLE;
        Serial.println("[WiFi] Scan timeout - will retry");
      }
      break;
    }

    // Scan completed: process results
    if (result >= 0)
    {
      // Store up to 10 networks
      wifiNetworkCount = min(result, 10);
      for (int i = 0; i < wifiNetworkCount; i++)
      {

        wifiSSID[i] = ""; // Clear previous SSID to avoid leftover data if new scan has fewer networks

        wifiSSID[i] = WiFi.SSID(i);
        wifiRSSI[i] = WiFi.RSSI(i);
      }

      WiFi.scanDelete(); // Free memory
      buzzer.playOnceTone(1000, 50);
      Serial.printf("[WiFi] Scan complete: %d networks found\n", wifiNetworkCount);
    }
    else
    {
      // Scan failed
      wifiNetworkCount = -1;
      WiFi.scanDelete();
      Serial.println("[WiFi] Scan error - operation failed");
    }

    lastWifiScan = now;
    wifiScanState = WIFI_DONE;
    break;
  }

  case WIFI_DONE:
    // Display results until it's time to scan again
    if (now - lastWifiScan >= WIFI_SCAN_INTERVAL)
    {
      wifiScanState = WIFI_IDLE;
    }
    break;
  }
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
    // Show animated scanning state
    WifiScanAnim.update();
    WifiScanAnim.draw(u8g2);

    // Animated progress indicator (dots)
    unsigned long elapsed = millis() - scanStartTime;
    int dotCount = (elapsed / 400) % 4;
    char scanMsg[20];
    snprintf(scanMsg, sizeof(scanMsg), "Scanning%s",
             dotCount == 0 ? "." : dotCount == 1 ? ".."
                               : dotCount == 2   ? "..."
                                                 : "....");
    u8g2.setFont(u8g2_font_5x8_tf);
    int centerX = FindCenterX(u8g2.getStrWidth(scanMsg));
    u8g2.drawStr(centerX, 54, scanMsg);
    break;
  }

  case WIFI_DONE:
  {
    // Display scan results
    u8g2.setCursor(5, 12);
    u8g2.print("WiFi: ");
    u8g2.print(wifiNetworkCount);
    u8g2.print(" found");

    u8g2.setFont(u8g2_font_6x10_mf);

    if (wifiNetworkCount < 0)
    {
      // Scan error
      u8g2.setCursor(5, 40);
      u8g2.print("Scan Error");
    }
    else if (wifiNetworkCount == 0)
    {
      // No networks found
      u8g2.setCursor(5, 40);
      u8g2.print("No networks found");
    }
    else
    {
      // Display up to 4 networks with SSID and signal strength
      int yPos = 24;
      for (int i = 0; i < min(4, wifiNetworkCount); i++)
      {
        u8g2.setCursor(2, yPos);

        // Truncate long SSIDs
        char networkName[16];
        wifiSSID[i].toCharArray(networkName, sizeof(networkName));

        if (strlen(networkName) > 15)
          networkName[15] = '\0';

        u8g2.print(networkName);
        u8g2.print(" ");
        u8g2.print(wifiRSSI[i]); // Signal strength in dBm
        u8g2.print("dB");
        yPos += 11;
      }
    }
    break;
  }
  }

  u8g2.sendBuffer();
}

// ============================================================================
//  DINO RUNNER GAME (T-Rex Style Endless Runner)
// ============================================================================
// Difficulty Progression:
//   Easy  → 2 obstacles, initial speed=3px/frame, max speed=6px/frame
//   Med   → 3 obstacles, initial speed=4px/frame, max speed=8px/frame
//   Hard  → 4 obstacles, initial speed=5px/frame, max speed=10px/frame, faster acceleration

void initDinoGame()
{
  int difficulty = getDiff();

  dinoObstacleCount = 2 + difficulty; // 2 / 3 / 4
  dinoObstacleSpeed = 3 + difficulty; // 3 / 4 / 5
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

  Serial.printf("[DINO] Difficulty=%d | Count=%d | Initial Speed=%dpx/frame\n",
                difficulty, dinoObstacleCount, gameSpeed);
}

void updateDinoGame()
{
  int difficulty = getDiff();
  int maxGameSpeed = 6 + (difficulty * 2);                     // 6 / 8 / 10
  int speedIncreaseInterval = (difficulty == 2) ? 3000 : 5000; // Hard: faster acceleration

  // Handle jump input
  if (UPButtonShortPressedGame)
  {
    if (!isJumping)
    {
      dinoVelY = JUMP_STRENGTH;
      isJumping = true;
      isDiving = false;
      buzzer.playOnceTone(1000, 50);
    }
  }

  // Handle dive input (only while airborne)
  if (DOWNButtonShortPressedGame && isJumping && !isDiving)
  {
    dinoVelY = DIVE_SPEED;
    isDiving = true;
    buzzer.playOnceTone(600, 50);
  }

  // Apply gravity or maintain dive speed
  dinoVelY = isDiving ? DIVE_SPEED : dinoVelY + GRAVITY;
  dinoY += dinoVelY;

  // Landing detection
  if (dinoY >= GROUND_Y)
  {
    dinoY = GROUND_Y;
    dinoVelY = 0;
    isJumping = false;
    isDiving = false;
  }

  // Update obstacle positions and check collisions
  for (int i = 0; i < dinoObstacleCount; i++)
  {
    dinoObstacles[i].x -= gameSpeed;

    // Recycle off-screen obstacles
    if (dinoObstacles[i].x < -OBS_WIDTH)
    {
      dinoObstacles[i].x = SCREEN_WIDTH + random(20, 60);
      dinoObstacles[i].active = true;
      dinoScore++;
    }

    // Collision detection
    if (dinoObstacles[i].active &&
        dinoObstacles[i].x < DINO_X + DINO_WIDTH &&
        dinoObstacles[i].x + OBS_WIDTH > DINO_X &&
        dinoY + DINO_HEIGHT > dinoObstacles[i].y)
    {
      // Game over: reset with new high score if applicable
      if (dinoScore > dinoHighScore)
        dinoHighScore = dinoScore;
      initDinoGame();
      buzzer.playOnceTone(400, 300);
      delay(300);
    }
  }

  // Progressive difficulty: increase speed over time
  if (millis() - lastSpeedIncrease > (unsigned long)speedIncreaseInterval)
  {
    if (gameSpeed < maxGameSpeed)
      gameSpeed++;
    lastSpeedIncrease = millis();
  }

  delay(30);
}

void drawDinoGame()
{
  u8g2.clearBuffer();

  // Draw ground line
  u8g2.drawHLine(0, GROUND_Y + DINO_HEIGHT + 1, SCREEN_WIDTH);

  // Draw dino character
  u8g2.drawBitmap(DINO_X, dinoY, 1, 8, dino_bitmap);

  // Draw dive indicator triangle
  if (isDiving)
    u8g2.drawTriangle(DINO_X + 4, dinoY - 5, DINO_X + 2, dinoY - 2, DINO_X + 6, dinoY - 2);

  // Draw obstacles
  for (int i = 0; i < dinoObstacleCount; i++)
  {
    if (dinoObstacles[i].active && dinoObstacles[i].x < SCREEN_WIDTH)
      u8g2.drawBox(dinoObstacles[i].x, dinoObstacles[i].y, OBS_WIDTH, OBS_HEIGHT);
  }

  // Draw UI: Score and High Score
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

// ============================================================================
//  GEOMETRY DASH GAME (Flappy Bird Style)
// ============================================================================
// Difficulty Progression:
//   Easy  → pillar speed=2px/frame, min gap=30px
//   Med   → pillar speed=3px/frame, min gap=24px
//   Hard  → pillar speed=4px/frame, min gap=18px, more frequent obstacles

void initGeoDashGame()
{
  int difficulty = getDiff();

  gdPillarSpeed = 2 + difficulty;   // 2 / 3 / 4
  gdGapMin = 30 - (difficulty * 6); // 30 / 24 / 18

  cubeX = 30;
  cubeY = SCREEN_HEIGHT / 2;
  cubeVelY = 0;
  cubeIsFlying = false;
  gdScore = 0;

  int pillarSpacing = 60 - (difficulty * 10); // 60 / 50 / 40 (x-axis spacing)

  for (int i = 0; i < MAX_GD_OBSTACLES; i++)
  {
    gdObstacles[i].x = SCREEN_WIDTH + (i * pillarSpacing);
    gdObstacles[i].gapY = random(gdGapMin + 5, SCREEN_HEIGHT - gdGapMin - 5);
    gdObstacles[i].gapHeight = random(gdGapMin, gdGapMin + 12);
    gdObstacles[i].active = true;
  }

  Serial.printf("[GEODASH] Difficulty=%d | Speed=%dpx/frame | Gap Min=%dpx\n",
                difficulty, gdPillarSpeed, gdGapMin);
}

void updateGeoDashGame()
{
  int difficulty = getDiff();
  int pillarSpacing = 60 - (difficulty * 10);

  // Handle jump input
  if (UPButtonShortPressedGame)
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

  // Apply gravity
  cubeVelY += GD_GRAVITY;
  cubeY += cubeVelY;

  // Game over: hit top or bottom
  if (cubeY <= 0 || cubeY >= SCREEN_HEIGHT - CUBE_SIZE)
  {
    if (gdScore > gdHighScore)
      gdHighScore = gdScore;
    initGeoDashGame();
    buzzer.playOnceTone(400, 300);
    delay(300);
    return;
  }

  // Update pillar positions and check collisions
  for (int i = 0; i < MAX_GD_OBSTACLES; i++)
  {
    gdObstacles[i].x -= gdPillarSpeed;

    // Recycle off-screen pillars
    if (gdObstacles[i].x < -PILLAR_WIDTH)
    {
      gdObstacles[i].x = SCREEN_WIDTH + random(10, pillarSpacing);
      gdObstacles[i].gapY = random(gdGapMin + 5, SCREEN_HEIGHT - gdGapMin - 5);
      gdObstacles[i].gapHeight = random(gdGapMin, gdGapMin + 12);
      gdObstacles[i].active = true;
      gdScore++;
      buzzer.playOnceTone(800, 50);
    }

    // Collision detection: check if cube passes through the gap
    if (gdObstacles[i].active &&
        gdObstacles[i].x < cubeX + CUBE_SIZE &&
        gdObstacles[i].x + PILLAR_WIDTH > cubeX)
    {
      int gapTop = gdObstacles[i].gapY - (gdObstacles[i].gapHeight / 2);
      int gapBottom = gdObstacles[i].gapY + (gdObstacles[i].gapHeight / 2);

      // Game over: cube doesn't fit through gap
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

  // Draw cube
  u8g2.drawBitmap(cubeX, cubeY, 1, 6, cube_bitmap);

  // Draw pillars with gaps
  for (int i = 0; i < MAX_GD_OBSTACLES; i++)
  {
    if (gdObstacles[i].active && gdObstacles[i].x < SCREEN_WIDTH && gdObstacles[i].x > -PILLAR_WIDTH)
    {
      int gapTop = gdObstacles[i].gapY - (gdObstacles[i].gapHeight / 2);
      int gapBottom = gdObstacles[i].gapY + (gdObstacles[i].gapHeight / 2);

      // Top pillar
      u8g2.drawBox(gdObstacles[i].x, 0, PILLAR_WIDTH, gapTop);

      // Bottom pillar
      u8g2.drawBox(gdObstacles[i].x, gapBottom, PILLAR_WIDTH, SCREEN_HEIGHT - gapBottom);
    }
  }

  // Draw UI: Score and High Score
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

// ============================================================================
//  CREDITS DISPLAY
// ============================================================================
// Scrollable credits screen with role and name information

void showCredit()
{
  if (!RunOnce)
  {
    u8g2.clearBuffer();
    RunOnce = true;
  }

  // Navigate credits: Up button
  if (UPButtonShortPressedGame)
  {
    UPButtonShortPressedGame = false;
    currentCreditIndex = (currentCreditIndex - 1 + totalCredits) % totalCredits;
    u8g2.clearBuffer();
  }

  // Navigate credits: Down button
  if (DOWNButtonShortPressedGame)
  {
    DOWNButtonShortPressedGame = false;
    currentCreditIndex = (currentCreditIndex + 1) % totalCredits;
    u8g2.clearBuffer();
  }

  int centerNameX = FindCenterX(u8g2.getStrWidth(credits[currentCreditIndex].name));

  u8g2.setFont(u8g2_font_7x14B_mf);
  u8g2.drawStr(5, 15, credits[currentCreditIndex].role);

  u8g2.setFont(u8g2_font_7x14_mf);
  u8g2.drawStr(centerNameX, 40, credits[currentCreditIndex].name);

  u8g2.drawBitmap(0, 1, 16, 21, epd_bitmap__Selected_outline);
  u8g2.sendBuffer();
}

// ============================================================================
//  SETTINGS MENU
// ============================================================================
// User configurable settings: Brightness, Difficulty, and Volume
// Settings are persisted to EEPROM storage

void applySettings()
{
  // Apply brightness setting to display
  u8g2.setContrast(BrightnessLevelValues[currentSettings.brightnessIndex]);

  Serial.printf("[SETTINGS] Applied → Brightness=%d | Difficulty=%s | Volume=%s\n",
                BrightnessLevelValues[currentSettings.brightnessIndex],
                DifficultyLevelLabels[currentSettings.difficultyIndex],
                VolumeLevelLabels[currentSettings.volumeIndex]);
}

// Helper: Cycle brightness level (0-4)
void toggleBrightnessSetting()
{
  if (UPButtonShortPressedGame)
  {
    UPButtonShortPressedGame = false;
    currentSettings.brightnessIndex = (currentSettings.brightnessIndex - 1 + 5) % 5;
  }
  else if (DOWNButtonShortPressedGame)
  {
    DOWNButtonShortPressedGame = false;
    currentSettings.brightnessIndex = (currentSettings.brightnessIndex + 1) % 5;
  }
}

// Helper: Cycle difficulty level (0-2: Easy/Med/Hard)
void toggleDifficultySetting()
{
  if (UPButtonShortPressedGame)
  {
    UPButtonShortPressedGame = false;
    currentSettings.difficultyIndex = (currentSettings.difficultyIndex - 1 + 3) % 3;
  }
  else if (DOWNButtonShortPressedGame)
  {
    DOWNButtonShortPressedGame = false;
    currentSettings.difficultyIndex = (currentSettings.difficultyIndex + 1) % 3;
  }
}

// Helper: Cycle volume level (0-4: Mute to Max)
void toggleSoundSetting()
{
  if (UPButtonShortPressedGame)
  {
    UPButtonShortPressedGame = false;
    currentSettings.volumeIndex = (currentSettings.volumeIndex - 1 + 5) % 5;
  }
  else if (DOWNButtonShortPressedGame)
  {
    DOWNButtonShortPressedGame = false;
    currentSettings.volumeIndex = (currentSettings.volumeIndex + 1) % 5;
  }
}

// Play feedback tone based on current setting value
void PlaySettingTone()
{
  switch (SelectedSetting)
  {
  case 0: // Brightness: ascending tones E5-C6
    buzzer.playOnceTone(currentSettings.brightnessIndex == 0 ? NOTE_E5 : currentSettings.brightnessIndex == 1 ? NOTE_F5
                                                                     : currentSettings.brightnessIndex == 2   ? NOTE_G5
                                                                     : currentSettings.brightnessIndex == 3   ? NOTE_A5
                                                                     : currentSettings.brightnessIndex == 4   ? NOTE_B5
                                                                                                              : NOTE_C6,
                        80);
    break;
  case 1: // Difficulty: E5 (Easy), F5 (Med), G5 (Hard)
    buzzer.playOnceTone(currentSettings.difficultyIndex == 0 ? NOTE_E5 : currentSettings.difficultyIndex == 1 ? NOTE_F5
                                                                                                              : NOTE_G5,
                        80);
    break;
  case 2: // Volume: beep if not mute
    if (VolumeLevelValues[currentSettings.volumeIndex] > 0)
      buzzer.playOnceTone(NOTE_G5, 80);
    break;
  }
}

void launchSelectedSetting()
{
  currentCondition = ON_ELEMENT;
  buzzer.playOnceTone(1200, 100);
}

void showSettings()
{
  if (currentCondition == ON_ELEMENT)
  {
    // In element editing mode: adjust selected setting value
    if (EnterButtonShortPressedGame)
      EnterButtonShortPressedGame = false;

    DrawSettingElement(SelectedSetting);

    if (DOWNButtonShortPressedGame || UPButtonShortPressedGame)
    {
      // Update selected setting
      switch (SelectedSetting)
      {
      case 0:
        toggleBrightnessSetting();
        settingsMgr.setBrightness(currentSettings.brightnessIndex);
        break;
      case 1:
        toggleDifficultySetting();
        settingsMgr.setDifficulty(currentSettings.difficultyIndex);
        break;
      case 2:
        toggleSoundSetting();
        settingsMgr.setVolume(currentSettings.volumeIndex);
        break;
      }

      PlaySettingTone();
      applySettings();
    }
  }
  else
  {
    // In menu selection mode: choose which setting to edit
    if (EnterButtonShortPressedGame)
    {
      EnterButtonShortPressedGame = false;
      launchSelectedSetting();
      return;
    }

    if (UPButtonShortPressedGame)
    {
      UPButtonShortPressedGame = false;
      SelectedSetting = (SelectedSetting - 1 + NUM_SETTING_MENU) % NUM_SETTING_MENU;
      SETTING_Selected_outlineY = SelectedSetting * SETTING_ITEM_HEIGHT;
      buzzer.playOnceTone(800, 50);
    }

    if (DOWNButtonShortPressedGame)
    {
      DOWNButtonShortPressedGame = false;
      SelectedSetting = (SelectedSetting + 1 + NUM_SETTING_MENU) % NUM_SETTING_MENU;
      SETTING_Selected_outlineY = SelectedSetting * SETTING_ITEM_HEIGHT;
      buzzer.playOnceTone(800, 50);
    }

    DrawSettingMenu();
  }
}

// Render detailed settings editor for single setting
void DrawSettingElement(int elementIndex)
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_8x13B_tf);

  int barWidth;
  const char *valueLabel;
  int settingNameCenterX = FindCenterX(u8g2.getStrWidth(settingNames[elementIndex]), 128);

  // Determine values based on selected setting
  switch (elementIndex)
  {
  case 0: // Brightness
    u8g2.drawStr(settingNameCenterX, 14, settingNames[0]);
    barWidth = map(currentSettings.brightnessIndex, 0, 4, 0, 90);
    valueLabel = BrightnessLevelLabels[currentSettings.brightnessIndex];
    break;
  case 1: // Difficulty
    u8g2.drawStr(settingNameCenterX, 14, settingNames[1]);
    barWidth = map(currentSettings.difficultyIndex, 0, 2, 0, 90);
    valueLabel = DifficultyLevelLabels[currentSettings.difficultyIndex];
    break;
  default: // Volume
    u8g2.drawStr(settingNameCenterX, 14, settingNames[2]);
    barWidth = map(currentSettings.volumeIndex, 0, 4, 0, 90);
    valueLabel = VolumeLevelLabels[currentSettings.volumeIndex];
    break;
  }

  // Draw progress bar
  int labelCenterX = FindCenterX(u8g2.getStrWidth(valueLabel), 32) + 23;
  u8g2.drawFrame(17, 28, 94, 20);
  if (barWidth > 0)
    u8g2.drawBox(19, 30, barWidth, 16);

  // Draw value label
  u8g2.setBitmapMode(0);
  u8g2.drawFrame(21, 20, 32, 16);
  u8g2.setFont(u8g2_font_7x13_mf);
  u8g2.drawStr(labelCenterX, 32, valueLabel);
  u8g2.setBitmapMode(1);

  // Draw instructions
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(0, 56, "Press: Change Value");
  u8g2.drawStr(0, 64, "Hold: Back to Menu");

  u8g2.sendBuffer();
}

// Render settings menu selection screen
void DrawSettingMenu()
{
  u8g2.firstPage();
  do
  {
    // Draw selection highlight
    u8g2.drawBitmap(0, SETTING_Selected_outlineY, 128 / 8, 21, setting_menu_bitmap__Icon_selectedOutline);

    // Item 0: Brightness
    u8g2.setDrawColor(SelectedSetting == 0 ? 0 : 1);
    u8g2.setFont(SelectedSetting == 0 ? u8g2_font_7x14B_mf : u8g2_font_7x14_mf);
    u8g2.drawStr(27, 16, setting_menu_bitmap_allArray_names[0]);
    u8g2.setDrawColor(1);
    u8g2.drawBitmap(3, 2, 2, 16, setting_menu_bitmap_allArray[0]);

    // Item 1: Difficulty
    u8g2.setDrawColor(SelectedSetting == 1 ? 0 : 1);
    u8g2.setFont(SelectedSetting == 1 ? u8g2_font_7x14B_mf : u8g2_font_7x14_mf);
    u8g2.drawStr(27, 38, setting_menu_bitmap_allArray_names[1]);
    u8g2.setDrawColor(1);
    u8g2.drawBitmap(3, 24, 2, 16, setting_menu_bitmap_allArray[1]);

    // Item 2: Volume
    u8g2.setDrawColor(SelectedSetting == 2 ? 0 : 1);
    u8g2.setFont(SelectedSetting == 2 ? u8g2_font_7x14B_mf : u8g2_font_7x14_mf);
    u8g2.drawStr(27, 58, setting_menu_bitmap_allArray_names[2]);
    u8g2.setDrawColor(1);
    u8g2.drawBitmap(3, 46, 2, 16, setting_menu_bitmap_allArray[2]);
  } while (u8g2.nextPage());
}

void launchSelectedIrMenu()
{
  currentCondition = ON_ELEMENT;
  buzzer.playOnceTone(1200, 100);
}

// ============================================================================
//  IR REMOTE CONTROL CLONING AND TRANSMISSION
// ============================================================================
// Two-mode operation: Capture IR signals from existing remotes or transmit
// previously captured codes. Supports multiple signal slots for storage.

void IRClonning()
{
  // In element mode: perform capture or transmit operation
  if (currentCondition == ON_ELEMENT)
  {
    switch (SelectedIrMenu)
    {
    // ════════════════════════════════════════════════════════
    // IR CAPTURE MODE: Receive and store IR signals
    // ════════════════════════════════════════════════════════
    case 0:
    {
      IRCaptureResult captureResult = irManager.update();

      // Feedback: signal captured
      if (captureResult == IR_RESULT_OK)
      {
        buzzer.playOnceTone(1200, 80);
        CaptureStartTime = millis();
      }

      u8g2.clearBuffer();
      IRCaptureAnim.update();
      IRCaptureAnim.draw(u8g2);

      u8g2.setFont(u8g2_font_5x8_tf);

      if (captureResult == IR_RESULT_OK || irManager.hasSignal())
      {
        // Display captured signal information
        char line1[24], line2[20];
        snprintf(line1, sizeof(line1), "OK! %s",
                 irManager.describeSelected().c_str());
        snprintf(line2, sizeof(line2), "Slot %s saved",
                 irManager.slotLabel().c_str());

        int centerX1 = FindCenterX(u8g2.getStrWidth(line1));
        int centerX2 = FindCenterX(u8g2.getStrWidth(line2));
        u8g2.drawStr(centerX1, 54, line1);
        u8g2.drawStr(centerX2, 63, line2);
      }
      else
      {
        // Waiting for signal: show animated progress
        unsigned long elapsedTime = millis() - CaptureStartTime;
        int progressDots = (elapsedTime / 400) % 4;
        char statusMsg[20];
        snprintf(statusMsg, sizeof(statusMsg), "Capturing%s",
                 progressDots == 0 ? "." : progressDots == 1 ? ".."
                                       : progressDots == 2   ? "..."
                                                             : "....");
        int centerX = FindCenterX(u8g2.getStrWidth(statusMsg));
        u8g2.drawStr(centerX, 54, statusMsg);
      }

      u8g2.sendBuffer();
      break;
    }

    // ════════════════════════════════════════════════════════
    // IR TRANSMIT MODE: Send stored IR signals
    // ════════════════════════════════════════════════════════
    case 1:
    {
      // Send on Enter button press
      if (EnterButtonShortPressedGame)
      {
        EnterButtonShortPressedGame = false;
        IRSendResult sendResult = irManager.send();

        if (sendResult == IR_SEND_OK)
          buzzer.playOnceTone(1000, 150); // Success beep
        else
          buzzer.playOnceTone(400, 300); // Error beep

        CaptureStartTime = millis();
        Serial.printf("[IR] Transmit result: %d\n", sendResult);
      }

      // Navigate to previous slot
      if (UPButtonShortPressedGame)
      {
        UPButtonShortPressedGame = false;
        irManager.previousSlot();
        buzzer.playOnceTone(800, 50);
        CaptureStartTime = millis();
      }

      // Navigate to next slot
      if (DOWNButtonShortPressedGame)
      {
        DOWNButtonShortPressedGame = false;
        irManager.nextSlot();
        buzzer.playOnceTone(800, 50);
        CaptureStartTime = millis();
      }

      u8g2.clearBuffer();
      IRSendingAnim.update();
      IRSendingAnim.draw(u8g2);

      u8g2.setFont(u8g2_font_5x8_tf);

      if (!irManager.hasSignal())
      {
        // No stored signal available
        const char *noSignalMsg = "No signal! Capture first";
        int centerX = FindCenterX(u8g2.getStrWidth(noSignalMsg));
        u8g2.drawStr(centerX, 54, noSignalMsg);
      }
      else
      {
        // Display current slot and instructions
        char signalDesc[20];
        char slotInfo[24];

        snprintf(signalDesc, sizeof(signalDesc), "%s",
                 irManager.describeSelected().c_str());
        snprintf(slotInfo, sizeof(slotInfo), "Slot %s Enter: send",
                 irManager.slotLabel().c_str());

        unsigned long elapsedTime = millis() - CaptureStartTime;
        int progressDots = (elapsedTime / 400) % 4;
        char statusMsg[20];
        snprintf(statusMsg, sizeof(statusMsg), "Ready%s",
                 progressDots == 0 ? "." : progressDots == 1 ? ".."
                                       : progressDots == 2   ? "..."
                                                             : "....");

        int centerX1 = FindCenterX(u8g2.getStrWidth(signalDesc));
        u8g2.drawStr(centerX1, 54, signalDesc);

        u8g2.setFont(u8g2_font_4x6_tf);
        int centerX2 = FindCenterX(u8g2.getStrWidth(slotInfo));
        u8g2.drawStr(centerX2, 63, slotInfo);
      }

      u8g2.sendBuffer();
      break;
    }

    default:
      break;
    }

    return;
  }

  // Menu selection mode: choose Capture or Transmit
  if (EnterButtonShortPressedGame)
  {
    EnterButtonShortPressedGame = false;
    launchSelectedIrMenu();
    return;
  }

  // Toggle between modes
  if (UPButtonShortPressedGame || DOWNButtonShortPressedGame)
  {
    UPButtonShortPressedGame = false;
    DOWNButtonShortPressedGame = false;
    SelectedIrMenu = (SelectedIrMenu + 1) % NUM_IR_MENU;
    IR_MENU_Selected_outlineY = SelectedIrMenu * IR_MENU_ITEM_HEIGHT;
    buzzer.playOnceTone(800, 50);
  }

  // Render IR mode selection menu
  u8g2.firstPage();
  do
  {
    u8g2.drawBitmap(0, IR_MENU_Selected_outlineY, 128 / 8, 21,
                    setting_menu_bitmap__Icon_selectedOutline);

    // Option 0: Capture mode
    u8g2.setDrawColor(SelectedIrMenu == 0 ? 0 : 1);
    u8g2.setFont(SelectedIrMenu == 0 ? u8g2_font_7x14B_mf : u8g2_font_7x14_mf);
    u8g2.drawStr(27, 16, IR_MENU[0]);
    u8g2.setDrawColor(1);
    u8g2.drawBitmap(3, 2, 2, 16, setting_menu_bitmap_allArray[0]);

    // Option 1: Transmit mode
    u8g2.setDrawColor(SelectedIrMenu == 1 ? 0 : 1);
    u8g2.setFont(SelectedIrMenu == 1 ? u8g2_font_7x14B_mf : u8g2_font_7x14_mf);
    u8g2.drawStr(27, 38, IR_MENU[1]);
    u8g2.setDrawColor(1);
    u8g2.drawBitmap(3, 24, 2, 16, setting_menu_bitmap_allArray[1]);

  } while (u8g2.nextPage());
}
