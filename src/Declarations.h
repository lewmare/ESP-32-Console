#ifndef Decelarations_H
#define Decelarations_H

// ==================== FUNCTION DECLARATIONS ====================
void drawMenu();
void handleButtonPress();
void launchGame(int gameIndex);
void exitGame();
void ToSubMenu();

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

#endif
