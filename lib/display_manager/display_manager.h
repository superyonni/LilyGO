// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#pragma once

#include <Arduino.h>

bool displayManagerBegin();
void displayManagerShowBoot();
void displayManagerShowMessage(const char *line1, const char *line2 = nullptr);
void displayManagerShowTransmit(int16_t rssi, float snr);
void displayManagerSleep();
