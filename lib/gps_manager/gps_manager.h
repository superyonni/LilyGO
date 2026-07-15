// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#pragma once

#include <Arduino.h>

struct GpsFix {
    bool valid;
    double latitude;
    double longitude;
    double altitude;
    uint8_t satellites;
};

bool gpsManagerBegin();
typedef void (*GpsProgressFn)(uint8_t satellites, uint32_t elapsedSec);
typedef bool (*GpsAbortFn)();
bool gpsManagerAcquireFix(GpsFix &fix, uint32_t timeoutMs, GpsProgressFn onProgress = nullptr,
                          GpsAbortFn shouldAbort = nullptr);
void gpsManagerSleep();
