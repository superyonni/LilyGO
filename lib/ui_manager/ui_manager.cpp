// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#include "ui_manager.h"

#include "config.h"

static constexpr uint32_t DEBOUNCE_MS = 70;
static constexpr uint32_t LONG_PRESS_MS = 900;

static volatile uint8_t clickQueue = 0;
static volatile uint32_t lastIsrMs = 0;
static volatile bool suppressNextClick = false;

static bool pressActive = false;
static uint32_t pressStartMs = 0;
static bool longPressFired = false;
static bool pendingLong = false;

static void enqueueClick()
{
    uint32_t now = millis();
    if ((now - lastIsrMs) < DEBOUNCE_MS) {
        return;
    }
    lastIsrMs = now;

    if (suppressNextClick) {
        suppressNextClick = false;
        return;
    }
    if (clickQueue < 8) {
        clickQueue++;
    }
}

static void onUserButtonIsr()
{
    // Activo LOW: al soltar el pin sube -> RISING = clic
    enqueueClick();
}

static void onTouchButtonIsr()
{
    // Activo HIGH: al soltar el pin baja -> FALLING = clic
    enqueueClick();
}

void uiManagerBegin()
{
    pinMode(User_Button_Pin, INPUT_PULLUP);
    pinMode(Touch_Button_Pin, INPUT_PULLDOWN);

    attachInterrupt(digitalPinToInterrupt(User_Button_Pin), onUserButtonIsr, RISING);
    attachInterrupt(digitalPinToInterrupt(Touch_Button_Pin), onTouchButtonIsr, FALLING);

    pressActive = false;
    longPressFired = false;
    pendingLong = false;
    suppressNextClick = false;
    clickQueue = 0;
}

static bool isPressed()
{
    return (digitalRead(User_Button_Pin) == LOW) || (digitalRead(Touch_Button_Pin) == HIGH);
}

void uiManagerPoll()
{
    bool pressed = isPressed();
    uint32_t now = millis();

    if (pressed && !pressActive) {
        pressActive = true;
        pressStartMs = now;
        longPressFired = false;
    }

    if (pressed && pressActive && !longPressFired) {
        if ((now - pressStartMs) >= LONG_PRESS_MS) {
            longPressFired = true;
            pendingLong = true;
            suppressNextClick = true;
            clickQueue = 0;
        }
    }

    if (!pressed && pressActive) {
        pressActive = false;
        longPressFired = false;
    }
}

bool uiManagerConsumeSinglePress()
{
    noInterrupts();
    if (clickQueue > 0) {
        clickQueue--;
        interrupts();
        return true;
    }
    interrupts();
    return false;
}

bool uiManagerConsumeDoublePress()
{
    if (pendingLong) {
        pendingLong = false;
        return true;
    }
    return false;
}

bool uiManagerIsPressed()
{
    return isPressed();
}
