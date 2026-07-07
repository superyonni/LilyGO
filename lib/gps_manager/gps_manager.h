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
bool gpsManagerAcquireFix(GpsFix &fix, uint32_t timeoutMs);
void gpsManagerSleep();
