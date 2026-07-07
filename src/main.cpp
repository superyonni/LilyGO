// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#include <Arduino.h>
#include <Wire.h>
#include <pcf8563.h>

#include "config.h"
#include "display_manager.h"
#include "gps_manager.h"
#include "lorawan_manager.h"

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

void setup()
{
    SerialMon.begin(MONITOR_SPEED);
    delay(500);

    SerialMon.println();
    SerialMon.println(F("=== T-Echo LoRaWAN Node ==="));
    SerialMon.println(F("Autor: Jonathan Garrido S."));

    enableBoardPower();
    Wire.begin();
    rtc.begin();

    currentState = NodeState::INIT;
}

void loop()
{
    switch (currentState) {
        case NodeState::INIT: {
            displayManagerBegin();
            displayManagerShowBoot();

            if (!gpsManagerBegin()) {
                displayManagerShowMessage("Error GPS", "Revisar modulo");
                delay(5000);
                break;
            }

            if (!lorawanManagerBegin()) {
                displayManagerShowMessage("Error LoRa", "Revisar radio");
                delay(5000);
                break;
            }

            batteryMv = readBatteryMv();
            currentState = NodeState::JOINING;
            break;
        }

        case NodeState::JOINING: {
            displayManagerShowMessage("Conectando a Red...", nullptr);
            digitalWrite(BlueLed_Pin, LOW);

            if (lorawanManagerJoin(JOIN_RETRY_MAX)) {
                digitalWrite(BlueLed_Pin, HIGH);
                currentState = NodeState::READ_SENSORS;
            } else {
                displayManagerShowMessage("JOIN fallido", "Reintentando...");
                delay(JOIN_RETRY_DELAY_MS);
            }
            break;
        }

        case NodeState::READ_SENSORS: {
            displayManagerShowMessage("Leyendo GPS...", nullptr);
            digitalWrite(GreenLed_Pin, LOW);

            if (!gpsManagerAcquireFix(lastFix, GPS_FIX_TIMEOUT_MS)) {
                lastFix.valid = false;
            }

            batteryMv = readBatteryMv();
            digitalWrite(GreenLed_Pin, HIGH);
            currentState = NodeState::TRANSMIT;
            break;
        }

        case NodeState::TRANSMIT: {
            displayManagerShowMessage("Transmitiendo...", nullptr);
            digitalWrite(RedLed_Pin, LOW);

            LoRaWanTxResult tx = lorawanManagerTransmit(lastFix, batteryMv);
            if (tx.success) {
                displayManagerShowTransmit(tx.rssi, tx.snr);
            } else {
                displayManagerShowMessage("TX fallida", nullptr);
            }

            digitalWrite(RedLed_Pin, HIGH);
            currentState = NodeState::SLEEP;
            break;
        }

        case NodeState::SLEEP: {
            if (DEEP_SLEEP_ENABLED) {
                enterDeepSleep(TX_INTERVAL_MS);
            } else {
                delay(TX_INTERVAL_MS);
                currentState = NodeState::READ_SENSORS;
            }
            break;
        }
    }
}
