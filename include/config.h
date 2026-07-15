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

// Botones T-Echo (NO usar RESET P0.18)
#define User_Button_Pin  _PINNUM(1, 10)  // activo LOW
#define Touch_Button_Pin _PINNUM(0, 11)  // activo HIGH

#define SerialMon Serial
#define SerialGPS Serial2
#define MONITOR_SPEED 115200

// --- ACTIVACION LoRaWAN ---
// 1 = ABP (recomendado ahora: sin JoinAccept). 0 = OTAA.
#ifndef LORAWAN_USE_ABP
#define LORAWAN_USE_ABP 1
#endif

// --- CREDENCIALES CHIRPSTACK (device techo-01) ---
#define LORAWAN_JOIN_EUI  0x0000000000000000ULL
#define LORAWAN_DEV_EUI   0x0A84041D4E2678ABULL
#define LORAWAN_APP_KEY   0x97, 0xA3, 0x25, 0x1B, 0x06, 0x3E, 0x6D, 0x91, \
                          0x40, 0x11, 0x67, 0xF8, 0xF9, 0x78, 0x60, 0xC4

// ABP: mismos valores en ChirpStack > techo-01 > Activation (perfil OTAA=OFF)
#define LORAWAN_DEV_ADDR  0x260B1A2BUL
#define LORAWAN_NWK_SKEY  0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18, \
                          0x29, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x90
#define LORAWAN_APP_SKEY  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, \
                          0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10

// Region AU915 — subbanda 2 (canales 8-15). Debe coincidir con el gateway.
#define LORAWAN_SUBBAND 2

// false = firmware completo (pantalla + GPS + LoRaWAN)
#ifndef JOIN_ONLY_TEST
#define JOIN_ONLY_TEST 0
#endif

// --- CONFIGURACION DE TIEMPOS ---
#define TX_INTERVAL_MS 30000UL       // 30 s entre envios (doble pulsacion = envio inmediato)
#define DEEP_SLEEP_ENABLED false     // true = bateria en campo prolongado
#define JOIN_RETRY_MAX 10
#define JOIN_RETRY_DELAY_MS 20000UL
#define GPS_FIX_TIMEOUT_MS 45000UL   // 45 s max GPS por ciclo (no bloquear LoRaWAN)
#define GPS_MIN_SATELLITES 3         // 3 sats minimo (campo)
#define GPS_QUICK_START 1            // 1 = omitir sondeo multi-baud al arrancar

// Umbrales señal LoRa (ajustar segun gateway)
#define RSSI_EXCELLENT_DBM  -70
#define RSSI_GOOD_DBM       -90
#define RSSI_WEAK_DBM       -110
#define SNR_EXCELLENT_DB    10.0f
#define SNR_GOOD_DB         0.0f
#define SNR_WEAK_DB         -5.0f

// Mapa: radio aprox. del primer fix (~900 m). Prueba campo caminando.
#define MAP_SPAN_DEG 0.008

#endif // CONFIG_H
