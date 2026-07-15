// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#pragma once

#include <Arduino.h>

void uiManagerBegin();
// Llamar en cada iteracion de loop().
void uiManagerPoll();
// true = clic corto (cambiar menu). Capturado por ISR; no se pierde durante e-paper.
bool uiManagerConsumeSinglePress();
// true = pulsacion larga (>=0.9 s) = envio inmediato.
bool uiManagerConsumeDoublePress();
bool uiManagerIsPressed();
