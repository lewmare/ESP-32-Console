#ifndef BITMAP_CONFIG_H
#define BITMAP_CONFIG_H
#include "Arduino.h"
#include <Bitmap/Bitmap.h>

// ==================== MENU CONFIGURATION ====================
const int NUM_ITEMS = 10;
const char menu_item[NUM_ITEMS][20] = {
    "PingPong",
    "CarObstacle",
    "NokiaSnake",
    "Angle",
    "Wifi",
    "Dino",
    "GeoBird",
    "InfraredClone",
    "Credit",
    "Settings"};

const unsigned char *bitmap_Icons[10] = {
    epd_bitmap__Icon_PingPong,
    epd_bitmap__Icon_CarObstacle,
    epd_bitmap__Icon_NokiaSnake,
    epd_bitmap__Icon_angle,
    epd_bitmap__icon_wifi,
    epd_bitmap__Icon_Dino,
    epd_bitmap__Icon_GeoDash,
    epd_bitmap__Icon_InfraredClonning,
    epd_bitmap__Icon_Credit,
    epd_bitmap__Icon_Setting};

const unsigned char *setting_menu_bitmap_allArray[3] = {
    setting_menu_bitmap__Icon_Brightness,
    setting_menu_bitmap__Icon_Difficulty,
    setting_menu_bitmap__Icon_Volume};

const int NUM_SETTING_MENU = 3;
const char setting_menu_bitmap_allArray_names[NUM_SETTING_MENU][20] = {
    "Brightness",
    "Difficulty",
    "Volume"};

#endif