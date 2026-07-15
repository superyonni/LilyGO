// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#include "gps_manager.h"

#include <TinyGPSPlus.h>

#include "config.h"

static TinyGPSPlus gps;

static bool probeGps()
{
    static const uint32_t rates[] = {9600, 19200, 38400, 57600, 115200, 4800};

    for (uint8_t i = 0; i < sizeof(rates) / sizeof(rates[0]); i++) {
        SerialGPS.end();
        delay(10);
        SerialGPS.begin(rates[i]);
        delay(50);

        uint32_t start = millis();
        while (millis() - start < 2000) {
            if (SerialGPS.available()) {
                String line = SerialGPS.readStringUntil('\n');
                line.trim();
                if (line.length() > 0 && line[0] == '$') {
                    if (rates[i] != 9600) {
                        for (int attempt = 0; attempt < 3; attempt++) {
                            SerialGPS.write("$PCAS01,1*1D\r\n");
                            delay(100);
                        }
                        SerialGPS.end();
                        delay(10);
                        SerialGPS.begin(9600);
                    }
                    return true;
                }
            }
            delay(10);
        }
    }

    SerialGPS.end();
    delay(10);
    SerialGPS.begin(9600);
    return false;
}

bool gpsManagerBegin()
{
    SerialMon.println(F("[GPS] Inicializando..."));

#ifndef PCA10056
    SerialGPS.setPins(Gps_Rx_Pin, Gps_Tx_Pin);
#endif
    SerialGPS.begin(9600);

    pinMode(Gps_Wakeup_Pin, OUTPUT);
    digitalWrite(Gps_Wakeup_Pin, HIGH);

    pinMode(Gps_Reset_Pin, OUTPUT);
    digitalWrite(Gps_Reset_Pin, HIGH);
    delay(10);
    digitalWrite(Gps_Reset_Pin, LOW);
    delay(10);
    digitalWrite(Gps_Reset_Pin, HIGH);

#if GPS_QUICK_START
    SerialMon.println(F("[GPS] Arranque rapido (9600 baud)"));
#else
    if (!probeGps()) {
        SerialMon.println(F("[GPS] Sin respuesta, usando 9600 baud"));
    }
#endif

    SerialGPS.write("$PCAS04,5*1C\r\n");
    delay(100);
    SerialGPS.write("$PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0*02\r\n");
    delay(100);
    SerialGPS.write("$PCAS11,3*1E\r\n");
    delay(100);

    return true;
}

bool gpsManagerAcquireFix(GpsFix &fix, uint32_t timeoutMs, GpsProgressFn onProgress,
                          GpsAbortFn shouldAbort)
{
    fix.valid = false;
    fix.latitude = 0.0;
    fix.longitude = 0.0;
    fix.altitude = 0.0;
    fix.satellites = 0;

    uint32_t start = millis();
    uint32_t lastUi = 0;

    while (millis() - start < timeoutMs) {
        if (shouldAbort != nullptr && shouldAbort()) {
            SerialMon.println(F("[GPS] Interrumpido por usuario"));
            if (gps.location.isValid()) {
                fix.valid = true;
                fix.latitude = gps.location.lat();
                fix.longitude = gps.location.lng();
                fix.altitude = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
                fix.satellites = gps.satellites.isValid() ? (uint8_t)gps.satellites.value() : 0;
            }
            break;
        }

        while (SerialGPS.available()) {
            gps.encode(SerialGPS.read());
        }

        fix.satellites = gps.satellites.isValid() ? (uint8_t)gps.satellites.value() : 0;

        uint32_t elapsed = (millis() - start) / 1000UL;
        if (onProgress != nullptr && (millis() - lastUi) >= 10000UL) {
            lastUi = millis();
            onProgress(fix.satellites, elapsed);
        }

        if (gps.location.isValid() && fix.satellites >= GPS_MIN_SATELLITES) {
            fix.valid = true;
            fix.latitude = gps.location.lat();
            fix.longitude = gps.location.lng();
            fix.altitude = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
            fix.satellites = gps.satellites.value();
            SerialMon.print(F("[GPS] Fix OK lat="));
            SerialMon.print(fix.latitude, 6);
            SerialMon.print(F(" lon="));
            SerialMon.println(fix.longitude, 6);
            return true;
        }

        delay(50);
    }

    SerialMon.println(F("[GPS] Timeout sin fix valido"));
    fix.satellites = gps.satellites.isValid() ? (uint8_t)gps.satellites.value() : 0;
    return false;
}

void gpsManagerSleep()
{
    SerialGPS.end();
    digitalWrite(Gps_Wakeup_Pin, LOW);
    pinMode(Gps_Reset_Pin, INPUT);
    pinMode(Gps_pps_Pin, INPUT);
    pinMode(Gps_Rx_Pin, INPUT);
    pinMode(Gps_Tx_Pin, INPUT);
}
