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

// --- CONFIGURACION DE CHIRPSTACK (OTAA) ---
static const uint64_t JOIN_EUI = 0x0000000000000000ULL;
static const uint64_t DEV_EUI = 0x70B3D57ED0050001ULL;
static uint8_t APP_KEY[] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};
static uint8_t NWK_KEY[] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

// Region LoRaWAN configurada en lorawan_manager (US915 subbanda 2 por defecto)
#define LORAWAN_SUBBAND 2

// --- CONFIGURACION DE TIEMPOS ---
#define TX_INTERVAL_MS 300000UL
#define DEEP_SLEEP_ENABLED true
#define JOIN_RETRY_MAX 5
#define JOIN_RETRY_DELAY_MS 15000UL
#define GPS_FIX_TIMEOUT_MS 120000UL
#define GPS_MIN_SATELLITES 4

#endif // CONFIG_H
