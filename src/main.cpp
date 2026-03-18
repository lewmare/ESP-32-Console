#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>

// ==================== STORAGE MANAGER=====================

#include "StorageManager.h"

SettingsManager settingsMgr;

// ===================== SETTING SETUP =====================

// ==================== IR SETUP =========================
#include "IR_Clone.h"

IRManager irManager;

// ==================== INISIALISASI ====================
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
Adafruit_MPU6050 mpu;

#include "Config.h"

// =========== Bitmap Animator (sistem modular baru) =============
#include "Bitmap/Bitmap_Config.h"
#include "Bitmap/Bitmap_anim.h"

BitmapAnim StartupAnim;
BitmapAnim WifiScanAnim;
BitmapAnim IRCaptureAnim;
BitmapAnim IRSendingAnim;

#include "Declarations.h"

// ==================== BUZZER ENGINE SETUP ====================
#include "Buzzer/BuzzerEngine.h"
#include "Buzzer/BuzzerMelody.h"

BuzzerEngine buzzer;

// ==================== SETUP ====================
void setup()
{
  Wire.begin(21, 22);
  Serial.begin(921600);

  settingsMgr.begin();

  currentSettings.brightnessIndex = settingsMgr.getBrightness();
  currentSettings.difficultyIndex = settingsMgr.getDifficulty();
  currentSettings.volumeIndex = settingsMgr.getVolume();

  applySettings();

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(EnterButton, INPUT_PULLUP);
  pinMode(UPButton, INPUT_PULLUP);
  pinMode(DOWNButton, INPUT_PULLUP);

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
  StartupAnim.init((const uint8_t *)Startup_frames, 48, 48, Startup_frames_COUNT, 21, 40, 8, ANIM_LOOP, ANIM_DIR_H, true, ANIM_MSB);
  WifiScanAnim.init((const uint8_t *)WifiScanning_frames, 32, 32, WifiScanning_frames_COUNT, 21, 48, 8, ANIM_LOOP, ANIM_DIR_H, true, ANIM_MSB);
  IRCaptureAnim.init((const uint8_t *)IR_WAITINGCAPTURE_frames, 32, 32, IR_WAITINGCAPTURE_frames_COUNT, 21, 48, 8, ANIM_LOOP, ANIM_DIR_H, true, ANIM_MSB);
  IRSendingAnim.init((const uint8_t *)IR_SENDING_frames, 32, 32, IR_SENDING_frames_COUNT, 21, 48, 8, ANIM_LOOP, ANIM_DIR_H, true, ANIM_MSB);

  buzzer.init(BUZZER_PIN);
  buzzer.play(melody_Startup, duration_Startup, sizeof(melody_Startup) / sizeof(melody_Startup[0]));

  irManager.begin(IR_RECEIVE_PIN, IR_SEND_PIN);

  Serial.println("Gaming Console Started!");
}

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

    settingsMgr.printSettingsSummary();
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

    int Enterbutton_state = digitalRead(EnterButton);
    int UPButton_state = digitalRead(UPButton);
    int DOWNButton_state = digitalRead(DOWNButton);

    if (Enterbutton_state == LOW && !EnterPressed)
    {
      EnterPressed = true;
      startPressTime = millis();
    }
    if (Enterbutton_state == HIGH && EnterPressed)
    {
      unsigned long dur = millis() - startPressTime;
      EnterPressed = false;
      if (dur >= longPressLimit)
      {
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
        if (currentCondition != ON_ELEMENT)
        {
          EnterButtonShortPressedGame = true;
        }
      }
    }

    if (UPButton_state == LOW && !UP_Pressed)
    {
      UP_Pressed = true;
      UPButtonShortPressedGame = true;
    }
    else if (UPButton_state == HIGH && UP_Pressed)
    {
      UP_Pressed = false;
      UPButtonShortPressedGame = false;
    }

    if (DOWNButton_state == LOW && !DOWN_Pressed)
    {
      DOWN_Pressed = true;
      DOWNButtonShortPressedGame = true;
    }
    else if (DOWNButton_state == HIGH && DOWN_Pressed)
    {
      DOWN_Pressed = false;
      DOWNButtonShortPressedGame = false;
    }
  }
}

// ==================== MENU FUNCTIONS ====================
void handleButtonPress()
{
  int Enterbutton_state = digitalRead(EnterButton);
  int UPbutton_state = digitalRead(UPButton);
  int DOWNbutton_state = digitalRead(DOWNButton);

  if (UPbutton_state == LOW && !UP_Pressed)
  {
    UP_Pressed = true;
    item_selected = (item_selected - 1 + NUM_ITEMS) % NUM_ITEMS;
  }
  else if (UPbutton_state == HIGH && UP_Pressed)
  {
    UP_Pressed = false;
  }

  if (DOWNbutton_state == LOW && !DOWN_Pressed)
  {
    DOWN_Pressed = true;
    item_selected = (item_selected + 1) % NUM_ITEMS;
  }
  else if (DOWNbutton_state == HIGH && DOWN_Pressed)
  {
    DOWN_Pressed = false;
  }

  if (Enterbutton_state == LOW && !EnterPressed)
  {
    EnterPressed = true;
  }
  if (Enterbutton_state == HIGH && EnterPressed)
  {
    EnterPressed = false;
    launchGame(item_selected);
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
    ToSubMenu();
    break;
  case 9:
    ToSubMenu();
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
  // Reset WiFi scan state agar mulai bersih saat masuk lagi
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

  if (UPButtonShortPressedGame)
  {
    UPButtonShortPressedGame = false;
    Serial.println("Snake: UP Pressed");
    snakeDir = (snakeDir + 1) % 4;
  }

  if (millis() - lastSnakeMove < (unsigned long)SNAKE_SPEED)
    return;
  lastSnakeMove = millis();

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

  // Cek for food collision

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
      wifiNetworkCount = -1;
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
      if (wifiNetworkCount < 0)
      {
        u8g2.setCursor(5, 40);
        u8g2.print("Scan Failed" + String(wifiNetworkCount < 0 ? " (Error)" : ""));
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

  if (DOWNButtonShortPressedGame && isJumping && !isDiving)
  {
    dinoVelY = DIVE_SPEED;
    isDiving = true;
    buzzer.playOnceTone(600, 50);
  }

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

  if (UPButtonShortPressedGame)
  {
    UPButtonShortPressedGame = false;
    currentCreditIndex = (currentCreditIndex - 1 + totalCredits) % totalCredits;
    u8g2.clearBuffer();
  }

  if (DOWNButtonShortPressedGame)
  {
    DOWNButtonShortPressedGame = false;
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

void DrawIrMenuElement(int elementIndex)
{
  u8g2.clearBuffer();
  switch (elementIndex)
  {
  case 0:
  {
    IRCaptureAnim.update();
    IRCaptureAnim.draw(u8g2);

    unsigned long elapsed = millis() - CaptureStartTime;
    int dots = (elapsed / 400) % 4; // 0..3 titik bergantian
    char CaptureMsg[20];
    snprintf(CaptureMsg, sizeof(CaptureMsg), "Capturing%s",
             dots == 0 ? "." : dots == 1 ? ".."
                           : dots == 2   ? "..."
                                         : "....");
    u8g2.setFont(u8g2_font_5x8_tf);
    int cx = FindCenterX(u8g2.getStrWidth(CaptureMsg));
    u8g2.drawStr(cx, 54, CaptureMsg);
    break;
  }
  case 1:
  {
    IRSendingAnim.update();
    IRSendingAnim.draw(u8g2);

    unsigned long elapsed = millis() - CaptureStartTime;
    int dots = (elapsed / 400) % 4; // 0..3 titik bergantian
    char CaptureMsg[20];
    snprintf(CaptureMsg, sizeof(CaptureMsg), "Sending%s",
             dots == 0 ? "." : dots == 1 ? ".."
                           : dots == 2   ? "..."
                                         : "....");
    u8g2.setFont(u8g2_font_5x8_tf);
    int cx = FindCenterX(u8g2.getStrWidth(CaptureMsg));
    u8g2.drawStr(cx, 54, CaptureMsg);
    break;
  }
  }
  u8g2.sendBuffer();
}

// ==================== SETTING =========================

void applySettings()
{
  u8g2.setContrast(BrightnessLevelValues[currentSettings.brightnessIndex]);
  Serial.printf("Settings applied → Brightness:%d  Difficulty:%d  Volume:%d\n",
                BrightnessLevelValues[currentSettings.brightnessIndex],
                DifficultyLevelValues[currentSettings.difficultyIndex],
                VolumeLevelValues[currentSettings.volumeIndex]);
}

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

void PlaySettingTone()
{
  switch (SelectedSetting)
  {
  case 0:
    buzzer.playOnceTone(currentSettings.brightnessIndex == 0 ? NOTE_E5 : currentSettings.brightnessIndex == 1 ? NOTE_F5
                                                                     : currentSettings.brightnessIndex == 2   ? NOTE_G5
                                                                     : currentSettings.brightnessIndex == 3   ? NOTE_A5
                                                                     : currentSettings.brightnessIndex == 4   ? NOTE_B5
                                                                                                              : NOTE_C6,
                        80);
    break;
  case 1:

    buzzer.playOnceTone(currentSettings.difficultyIndex == 0 ? NOTE_E5 : currentSettings.difficultyIndex == 1 ? NOTE_F5
                                                                                                              : NOTE_G5,
                        80);

    break;
  case 2:
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
    DrawSettingElement(SelectedSetting);
    if (DOWNButtonShortPressedGame || UPButtonShortPressedGame)
    {
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
    return;
  }
  else
  {

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

void DrawSettingMenu()
{
  u8g2.firstPage();
  do
  {
    u8g2.drawBitmap(0, SETTING_Selected_outlineY, 128 / 8, 21, setting_menu_bitmap__Icon_selectedOutline);

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

void launchSelectedIrMenu()
{
  currentCondition = ON_ELEMENT;
  buzzer.playOnceTone(1200, 100);
}

void IRClonning()
{
  // ── Sub-menu: Capture / Send ──────────────────────────────
  if (currentCondition == ON_ELEMENT)
  {
    switch (SelectedIrMenu)
    {
    // ════════════════════════════════
    // CASE 0 — CAPTURE
    // ════════════════════════════════
    case 0:
    {
      // Jalankan IR receive logic setiap frame
      IRCaptureResult res = irManager.update();

      if (res == IR_RESULT_OK)
      {
        buzzer.playOnceTone(1200, 80); // feedback: sinyal masuk
        CaptureStartTime = millis();   // reset timer animasi teks
      }

      // Render: animasi + teks status
      u8g2.clearBuffer();
      IRCaptureAnim.update();
      IRCaptureAnim.draw(u8g2);

      // Baris teks animasi titik (Capturing... / Captured!)
      u8g2.setFont(u8g2_font_5x8_tf);
      if (res == IR_RESULT_OK || irManager.hasSignal())
      {
        // Tampilkan info sinyal yang baru direkam
        char line1[24], line2[20];
        snprintf(line1, sizeof(line1), "OK! %s",
                 irManager.describeSelected().c_str());
        snprintf(line2, sizeof(line2), "Slot %s saved",
                 irManager.slotLabel().c_str());
        int cx1 = FindCenterX(u8g2.getStrWidth(line1));
        int cx2 = FindCenterX(u8g2.getStrWidth(line2));
        u8g2.drawStr(cx1, 54, line1);
        u8g2.drawStr(cx2, 63, line2);
      }
      else
      {
        // Belum ada sinyal — animasi titik
        unsigned long elapsed = millis() - CaptureStartTime;
        int dots = (elapsed / 400) % 4;
        char msg[20];
        snprintf(msg, sizeof(msg), "Capturing%s",
                 dots == 0 ? "." : dots == 1 ? ".."
                               : dots == 2   ? "..."
                                             : "....");
        int cx = FindCenterX(u8g2.getStrWidth(msg));
        u8g2.drawStr(cx, 54, msg);
      }

      u8g2.sendBuffer();
      break;
    }

    // ════════════════════════════════
    // CASE 1 — SEND
    // ════════════════════════════════
    case 1:
    {
      // Kirim saat tombol pendek
      if (EnterButtonShortPressedGame)
      {
        EnterButtonShortPressedGame = false;
        IRSendResult res = irManager.send();
        if (res == IR_SEND_OK)
          buzzer.playOnceTone(1000, 150); // feedback: terkirim
        else
          buzzer.playOnceTone(400, 300); // feedback: kosong
        CaptureStartTime = millis();     // reset timer teks
      }

      if (UPButtonShortPressedGame)
      {
        UPButtonShortPressedGame = false;
        irManager.previousSlot();
        buzzer.playOnceTone(800, 50);
        CaptureStartTime = millis(); // reset timer teks
      }

      if (DOWNButtonShortPressedGame)
      {
        DOWNButtonShortPressedGame = false;
        irManager.nextSlot();
        buzzer.playOnceTone(800, 50);
        CaptureStartTime = millis(); // reset timer teks
      }

      // Render: animasi + teks status
      u8g2.clearBuffer();
      IRSendingAnim.update();
      IRSendingAnim.draw(u8g2);

      u8g2.setFont(u8g2_font_5x8_tf);
      if (!irManager.hasSignal())
      {
        // Tidak ada sinyal tersimpan
        const char *noSig = "No signal! Capture first";
        u8g2.drawStr(FindCenterX(u8g2.getStrWidth(noSig)), 54, noSig);
      }
      else
      {
        // Tampilkan info sinyal + instruksi
        char line1[20];
        char line2[24];

        snprintf(line1, sizeof(line1), "%s",
                 irManager.describeSelected().c_str());
        // Tampilkan slot irManager saat ini, dan instruksi untuk mengirim;
        snprintf(line2, sizeof(line2), "Slot %s Enter : send",
                 irManager.slotLabel().c_str());
        unsigned long elapsed = millis() - CaptureStartTime;
        int dots = (elapsed / 400) % 4;
        char sending[20];
        snprintf(sending, sizeof(sending), "Sending%s",
                 dots == 0 ? "." : dots == 1 ? ".."
                               : dots == 2   ? "..."
                                             : "....");
        int cx1 = FindCenterX(u8g2.getStrWidth(line1));
        u8g2.drawStr(cx1, 54, line1);

        u8g2.setFont(u8g2_font_4x6_tf);

        int cx2 = FindCenterX(u8g2.getStrWidth(line2));
        u8g2.drawStr(cx2, 63, line2);
      }

      u8g2.sendBuffer();
      break;
    }

    default:
      break;
    }

    return;
  }

  // ── Menu navigasi IR (Capture / Send) ────────────────────

  if (EnterButtonShortPressedGame)
  {
    EnterButtonShortPressedGame = false;
    launchSelectedIrMenu();
    return;
  }

  if (UPButtonShortPressedGame || DOWNButtonShortPressedGame)
  {
    UPButtonShortPressedGame = false;
    DOWNButtonShortPressedGame = false;
    SelectedIrMenu = (SelectedIrMenu + 1) % NUM_IR_MENU;
    IR_MENU_Selected_outlineY = SelectedIrMenu * IR_MENU_ITEM_HEIGHT;
    buzzer.playOnceTone(800, 50);
  }

  u8g2.firstPage();
  do
  {
    u8g2.drawBitmap(0, IR_MENU_Selected_outlineY, 128 / 8, 21,
                    setting_menu_bitmap__Icon_selectedOutline);

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