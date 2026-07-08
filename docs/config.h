// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#ifndef _PINNUM
#define _PINNUM(port, pin) ((port) * 32 + (pin))
#endif

// --- MAPEO DE PINES T-ECHO ---
#define ePaper_Miso _PINNUM(1, 6)
#define ePaper_Mosi _PINNUM(0, 29)
#define ePaper_Sclk _PINNUM(0, 31)
#define ePaper_Cs _PINNUM(0, 30)
#define ePaper_Dc _PINNUM(0, 28)
#define ePaper_Rst _PINNUM(0, 2)
#define ePaper_Busy _PINNUM(0, 3)
#define ePaper_Backlight _PINNUM(1, 11)

#define LoRa_Miso _PINNUM(0, 23)
#define LoRa_Mosi _PINNUM(0, 22)
#define LoRa_Sclk _PINNUM(0, 19)
#define LoRa_Cs _PINNUM(0, 24)
#define LoRa_Rst _PINNUM(0, 25)
#define LoRa_Dio1 _PINNUM(0, 20)
#define LoRa_Busy _PINNUM(0, 17)

#define Adc_Pin _PINNUM(0, 4)
#define SDA_Pin _PINNUM(0, 26)
#define SCL_Pin _PINNUM(0, 27)
#define RTC_Int_Pin _PINNUM(0, 16)

#define Gps_Rx_Pin _PINNUM(1, 9)
#define Gps_Tx_Pin _PINNUM(1, 8)
#define Gps_Wakeup_Pin _PINNUM(1, 2)
#define Gps_Reset_Pin _PINNUM(1, 5)
#define Gps_pps_Pin _PINNUM(1, 4)

#define Power_Enable_Pin _PINNUM(0, 12)
#define GreenLed_Pin _PINNUM(1, 1)
#define RedLed_Pin _PINNUM(1, 3)
#define BlueLed_Pin _PINNUM(0, 14)

#define SerialMon Serial
#define SerialGPS Serial2
#define MONITOR_SPEED 115200

// --- CONFIGURACION DE CHIRPSTACK (OTAA) - device techo-01 ---
// ChirpStack: Applications / LilyGO-TEcho / Devices / techo-01
//
// ChirpStack (hex string)     ->  C++ (correcto, NO cambiar)
// DevEUI  0a84041d4e2678ab    ->  0x0A84041D4E2678ABULL
// AppKey  b71f775e5a2bbf32471dfed62ce079a3  ->  bytes MSB abajo (coincide 1:1)
static const uint64_t JOIN_EUI = 0x0000000000000000ULL;
static const uint64_t DEV_EUI  = 0x0A84041D4E2678ABULL;
static uint8_t APP_KEY[] = {
    0xB7, 0x1F, 0x77, 0x5E, 0x5A, 0x2B, 0xBF, 0x32,
    0x47, 0x1D, 0xFE, 0xD6, 0x2C, 0xE0, 0x79, 0xA3
};
static uint8_t NWK_KEY[] = {
    0xB7, 0x1F, 0x77, 0x5E, 0x5A, 0x2B, 0xBF, 0x32,
    0x47, 0x1D, 0xFE, 0xD6, 0x2C, 0xE0, 0x79, 0xA3
};

// Region AU915 — subbanda debe coincidir con el gateway (2 = canales 8-15)
#define LORAWAN_SUBBAND 2

// --- CONFIGURACION DE TIEMPOS ---
#define TX_INTERVAL_MS 300000UL
#define DEEP_SLEEP_ENABLED false  // false = debug (USB siempre activo para monitor/upload)
#define JOIN_RETRY_MAX 5
#define JOIN_RETRY_DELAY_MS 15000UL
#define GPS_FIX_TIMEOUT_MS 120000UL
#define GPS_MIN_SATELLITES 4

#endif // CONFIG_H
