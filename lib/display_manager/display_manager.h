// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#pragma once

#include <Arduino.h>

#include "gps_manager.h"

enum class DisplayPage : uint8_t {
    OVERVIEW = 0,
    GPS_DATA,
    LORA_RF,
    MAP_VIEW,
    COUNT
};

struct DisplaySnapshot {
    bool networkOk;
    bool txOk;
    GpsFix fix;
    uint16_t batteryMv;
    int16_t rssi;
    float snr;
    uint32_t devAddr;
    uint32_t fCnt;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

bool displayManagerBegin();
void displayManagerShowBootAnimation();
void displayManagerShowMessage(const char *line1, const char *line2 = nullptr);
void displayManagerShowGpsProgress(uint8_t satellites, uint32_t elapsedSec);
void displayManagerSetSnapshot(const DisplaySnapshot &data);
void displayManagerSetPage(DisplayPage page);
DisplayPage displayManagerGetPage();
DisplayPage displayManagerNextPage();
void displayManagerRenderCurrentPage();
void displayManagerSetUserBrowsing(bool browsing);
bool displayManagerIsUserBrowsing();
void displayManagerSleep();
