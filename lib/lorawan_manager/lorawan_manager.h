// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#pragma once

#include <Arduino.h>

#include "gps_manager.h"

struct LoRaWanTxResult {
    bool success;
    int16_t rssi;
    float snr;
};

bool lorawanManagerBegin();
bool lorawanManagerJoin(uint8_t maxRetries);
LoRaWanTxResult lorawanManagerTransmit(const GpsFix &fix, uint16_t batteryMv);
uint32_t lorawanManagerGetFCntUp();
uint32_t lorawanManagerGetDevAddr();
void lorawanManagerSleep();
