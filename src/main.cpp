// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#include <Arduino.h>

#include "config.h"
#include "lorawan_manager.h"

#if JOIN_ONLY_TEST

void setup()
{
    pinMode(Power_Enable_Pin, OUTPUT);
    digitalWrite(Power_Enable_Pin, HIGH);

    SerialMon.begin(MONITOR_SPEED);
    uint32_t usbWait = millis();
    while (!SerialMon && (millis() - usbWait < 4000)) {
        delay(10);
    }
    delay(500);
    SerialMon.println();
    SerialMon.println(F("BOOT T-Echo LoRaWAN TEST"));
    SerialMon.flush();

    SerialMon.println(F("=== T-Echo LoRaWAN TEST (solo radio) ==="));
#if LORAWAN_USE_ABP
    SerialMon.println(F("Modo ABP | docs/chirpstack-abp-paso-a-paso.md"));
#else
    SerialMon.println(F("Modo OTAA | docs/chirpstack-techo-01.md"));
#endif
    SerialMon.flush();

    pinMode(BlueLed_Pin, OUTPUT);

    if (!lorawanManagerBegin()) {
        SerialMon.println(F("FATAL: radio no inicia"));
        while (true) {
            delay(1000);
        }
    }
}

void loop()
{
    digitalWrite(BlueLed_Pin, LOW);

    if (lorawanManagerJoin(JOIN_RETRY_MAX)) {
        digitalWrite(BlueLed_Pin, HIGH);
#if LORAWAN_USE_ABP
        SerialMon.println(F("*** ABP OK - enviando uplink ***"));
#else
        SerialMon.println(F("*** JOIN OK - enviando uplink ***"));
#endif

        GpsFix dummy = {false, 0.0, 0.0, 0.0, 0};
        LoRaWanTxResult tx = lorawanManagerTransmit(dummy, 3700);
        if (tx.success) {
            SerialMon.println(F("*** UPLINK OK ***"));
        } else {
            SerialMon.println(F("Uplink fallo tras join"));
        }

        while (true) {
            delay(60000);
            lorawanManagerTransmit(dummy, 3700);
        }
    }

    SerialMon.println(F("Reintento JOIN en 30s..."));
    delay(30000);
}

#else

#include <Wire.h>
#include <pcf8563.h>

#include "display_manager.h"
#include "gps_manager.h"
#include "ui_manager.h"

enum class NodeState {
    INIT,
    JOINING,
    READ_SENSORS,
    TRANSMIT,
    SLEEP
};

static NodeState currentState = NodeState::INIT;
static PCF8563_Class rtc;
static GpsFix lastFix = {false, 0.0, 0.0, 0.0, 0};
static uint16_t batteryMv = 0;
static bool networkReady = false;
static bool forceImmediateTx = false;
static int16_t lastRssi = 0;
static float lastSnr = 0.0f;
static bool lastTxOk = false;
static bool bootUplinkPending = true;
static bool mapAutoShown = false;
static DisplaySnapshot displaySnap = {};

static void logState(const __FlashStringHelper *name)
{
    SerialMon.print(F("[STATE] "));
    SerialMon.println(name);
}

static void updateDisplaySnapshot()
{
    RTC_Date now = rtc.getDateTime();
    displaySnap.networkOk = networkReady;
    displaySnap.txOk = lastTxOk;
    displaySnap.fix = lastFix;
    displaySnap.batteryMv = batteryMv;
    displaySnap.rssi = lastRssi;
    displaySnap.snr = lastSnr;
    displaySnap.devAddr = lorawanManagerGetDevAddr();
    displaySnap.fCnt = lorawanManagerGetFCntUp();
    displaySnap.hour = now.hour;
    displaySnap.minute = now.minute;
    displaySnap.second = now.second;
    displayManagerSetSnapshot(displaySnap);
}

static void refreshDisplaySnapshot()
{
    updateDisplaySnapshot();
    displayManagerRenderCurrentPage();
}

static void handleUiInput();

static uint16_t readBatteryMv()
{
    analogReadResolution(12);
    uint32_t raw = 0;
    for (uint8_t i = 0; i < 8; i++) {
        raw += analogRead(Adc_Pin);
        delay(5);
    }
    raw /= 8;
    return (uint16_t)((raw * 2UL * 3600UL) / 4096UL);
}

static void enableBoardPower()
{
    pinMode(Power_Enable_Pin, OUTPUT);
    digitalWrite(Power_Enable_Pin, HIGH);
    pinMode(GreenLed_Pin, OUTPUT);
    pinMode(RedLed_Pin, OUTPUT);
    pinMode(BlueLed_Pin, OUTPUT);
}

static void enterDeepSleep(uint32_t sleepMs)
{
    displayManagerSleep();
    gpsManagerSleep();
    lorawanManagerSleep();

    digitalWrite(Power_Enable_Pin, LOW);
    pinMode(Power_Enable_Pin, INPUT);

    Wire.begin();
    rtc.begin();

    RTC_Date now = rtc.getDateTime();
    uint32_t sleepSeconds = (sleepMs + 999UL) / 1000UL;
    if (sleepSeconds < 1) {
        sleepSeconds = 1;
    }

    uint32_t totalSeconds = (uint32_t)now.hour * 3600UL
        + (uint32_t)now.minute * 60UL
        + (uint32_t)now.second
        + sleepSeconds;

    uint8_t wakeDay = now.day + (totalSeconds / 86400UL);
    totalSeconds %= 86400UL;
    uint8_t wakeHour = totalSeconds / 3600UL;
    totalSeconds %= 3600UL;
    uint8_t wakeMinute = totalSeconds / 60UL;

    rtc.setAlarm(wakeHour, wakeMinute, wakeDay, PCF8563_NO_ALARM);
    rtc.enableAlarm();

    pinMode(RTC_Int_Pin, INPUT_PULLUP_SENSE);
    SerialMon.flush();
    SerialMon.end();

    uint8_t sdEnabled = 0;
    (void)sd_softdevice_is_enabled(&sdEnabled);
    if (sdEnabled) {
        sd_power_system_off();
    } else {
        NRF_POWER->SYSTEMOFF = 1;
    }

    while (true) {
        delay(1);
    }
}

static void onGpsProgress(uint8_t satellites, uint32_t elapsedSec)
{
    displayManagerShowGpsProgress(satellites, elapsedSec);
}

static bool onGpsAbort()
{
    handleUiInput();
    return forceImmediateTx;
}

static void pollDelay(uint32_t ms)
{
    uint32_t start = millis();
    while (millis() - start < ms) {
        handleUiInput();
        delay(10);
    }
}

static void handleUiInput()
{
    uiManagerPoll();

    if (uiManagerConsumeDoublePress()) {
        forceImmediateTx = true;
        digitalWrite(BlueLed_Pin, LOW);
        SerialMon.println(F("[UI] Hold largo -> envio inmediato"));
    }

    uint8_t clicks = 0;
    while (uiManagerConsumeSinglePress()) {
        clicks++;
        displayManagerNextPage();
    }

    if (clicks > 0) {
        displayManagerSetUserBrowsing(true);
        updateDisplaySnapshot();
        displayManagerRenderCurrentPage();
        digitalWrite(BlueLed_Pin, LOW);
        SerialMon.print(F("[UI] Menu "));
        SerialMon.print((unsigned)displayManagerGetPage() + 1);
        SerialMon.print('/');
        SerialMon.println((unsigned)DisplayPage::COUNT);
    }

    static uint32_t ledRestoreMs = 0;
    if (uiManagerIsPressed()) {
        digitalWrite(BlueLed_Pin, LOW);
        ledRestoreMs = millis() + 150;
    } else if (ledRestoreMs != 0 && millis() >= ledRestoreMs) {
        digitalWrite(BlueLed_Pin, HIGH);
        ledRestoreMs = 0;
    }
}

void setup()
{
    enableBoardPower();

    SerialMon.begin(MONITOR_SPEED);
    delay(500);

    SerialMon.println();
    SerialMon.println(F("=== T-Echo LoRaWAN Node ==="));

    Wire.begin();
    rtc.begin();

    uiManagerBegin();
    displayManagerBegin();
    displayManagerShowBootAnimation();

    currentState = NodeState::INIT;
}

void loop()
{
    handleUiInput();

    switch (currentState) {
        case NodeState::INIT: {
            logState(F("INIT"));
            displayManagerShowMessage("Iniciando radio", "techo-01");

            if (!lorawanManagerBegin()) {
                displayManagerShowMessage("Error LoRa", "Reintentando...");
                SerialMon.println(F("[INIT] Error LoRa"));
                pollDelay(5000);
                break;
            }

            gpsManagerBegin();
            batteryMv = readBatteryMv();
            currentState = NodeState::JOINING;
            break;
        }

        case NodeState::JOINING: {
            logState(F("JOINING"));
#if LORAWAN_USE_ABP
            displayManagerShowMessage("Activando ABP...", "techo-01");
#else
            displayManagerShowMessage("Conectando OTAA...", nullptr);
#endif
            digitalWrite(BlueLed_Pin, LOW);

            if (lorawanManagerJoin(JOIN_RETRY_MAX)) {
                digitalWrite(BlueLed_Pin, HIGH);
                networkReady = true;
#if LORAWAN_USE_ABP
                displayManagerShowMessage("ABP OK", bootUplinkPending ? "Enviando..." : "Leyendo sensores...");
#else
                displayManagerShowMessage("JOIN OK", bootUplinkPending ? "Enviando..." : "Leyendo sensores...");
#endif
                currentState = bootUplinkPending ? NodeState::TRANSMIT : NodeState::READ_SENSORS;
            } else {
                displayManagerShowMessage("Red fallida", "Reintentando...");
                pollDelay(JOIN_RETRY_DELAY_MS);
            }
            break;
        }

        case NodeState::READ_SENSORS: {
            logState(F("READ_SENSORS"));
            digitalWrite(GreenLed_Pin, LOW);

            if (!forceImmediateTx) {
                if (!gpsManagerAcquireFix(lastFix, GPS_FIX_TIMEOUT_MS, onGpsProgress, onGpsAbort)) {
                    if (!lastFix.valid) {
                        lastFix.valid = false;
                    }
                }
            } else {
                SerialMon.println(F("[GPS] Envio inmediato, omitiendo nueva adquisicion"));
            }

            // Primer FIX: saltar al mapa automaticamente
            if (lastFix.valid && !mapAutoShown) {
                mapAutoShown = true;
                displayManagerSetPage(DisplayPage::MAP_VIEW);
                displayManagerSetUserBrowsing(true);
                refreshDisplaySnapshot();
                SerialMon.println(F("[UI] FIX OK -> pantalla Mapa"));
            }

            batteryMv = readBatteryMv();
            digitalWrite(GreenLed_Pin, HIGH);
            currentState = NodeState::TRANSMIT;
            break;
        }

        case NodeState::TRANSMIT: {
            logState(F("TRANSMIT"));
            if (!displayManagerIsUserBrowsing()) {
                displayManagerShowMessage("Transmitiendo...", nullptr);
            }
            digitalWrite(RedLed_Pin, LOW);

            LoRaWanTxResult tx = lorawanManagerTransmit(lastFix, batteryMv);
            lastRssi = tx.rssi;
            lastSnr = tx.snr;
            lastTxOk = tx.success;

            refreshDisplaySnapshot();

            digitalWrite(RedLed_Pin, HIGH);
            bootUplinkPending = false;
            forceImmediateTx = false;
            currentState = NodeState::SLEEP;
            break;
        }

        case NodeState::SLEEP: {
            logState(F("SLEEP"));
            if (DEEP_SLEEP_ENABLED) {
                enterDeepSleep(TX_INTERVAL_MS);
            } else {
                digitalWrite(BlueLed_Pin, HIGH);
                uint32_t waitStart = millis();
                while (millis() - waitStart < TX_INTERVAL_MS) {
                    handleUiInput();
                    if (forceImmediateTx) {
                        break;
                    }
                    delay(10);
                }
                currentState = NodeState::READ_SENSORS;
            }
            break;
        }
    }
}

#endif
