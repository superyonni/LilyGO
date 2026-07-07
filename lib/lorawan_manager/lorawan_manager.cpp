// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#include "lorawan_manager.h"

#include <RadioLib.h>
#include <SPI.h>

#include "config.h"

static SPIClass *rfPort = nullptr;
static SX1262 *radio = nullptr;
static LoRaWANNode *node = nullptr;
static Module *radioModule = nullptr;

static int32_t encodeCoordinate(double value)
{
    return (int32_t)(value * 10000000.0);
}

static void packPayload(const GpsFix &fix, uint16_t batteryMv, uint8_t *payload, size_t &length)
{
    int32_t lat = fix.valid ? encodeCoordinate(fix.latitude) : 0;
    int32_t lon = fix.valid ? encodeCoordinate(fix.longitude) : 0;
    int16_t alt = fix.valid ? (int16_t)fix.altitude : 0;

    payload[0] = (lat >> 24) & 0xFF;
    payload[1] = (lat >> 16) & 0xFF;
    payload[2] = (lat >> 8) & 0xFF;
    payload[3] = lat & 0xFF;

    payload[4] = (lon >> 24) & 0xFF;
    payload[5] = (lon >> 16) & 0xFF;
    payload[6] = (lon >> 8) & 0xFF;
    payload[7] = lon & 0xFF;

    payload[8] = (alt >> 8) & 0xFF;
    payload[9] = alt & 0xFF;

    payload[10] = (batteryMv >> 8) & 0xFF;
    payload[11] = batteryMv & 0xFF;

    length = 12;
}

bool lorawanManagerBegin()
{
    SerialMon.println(F("[LoRa] Inicializando radio SX1262..."));

    rfPort = new SPIClass(NRF_SPIM3, LoRa_Miso, LoRa_Sclk, LoRa_Mosi);
    rfPort->begin();

    radioModule = new Module(LoRa_Cs, LoRa_Dio1, LoRa_Rst, LoRa_Busy, *rfPort, SPISettings());
    radio = new SX1262(radioModule);

    int16_t state = radio->begin();
    if (state != RADIOLIB_ERR_NONE) {
        SerialMon.print(F("[LoRa] begin() fallo: "));
        SerialMon.println(state);
        return false;
    }

    radio->setOutputPower(10);

    node = new LoRaWANNode(radio, &US915, LORAWAN_SUBBAND);
    node->beginOTAA(JOIN_EUI, DEV_EUI, NWK_KEY, APP_KEY);

    return true;
}

bool lorawanManagerJoin(uint8_t maxRetries)
{
    if (node == nullptr) {
        return false;
    }

    for (uint8_t attempt = 1; attempt <= maxRetries; attempt++) {
        SerialMon.print(F("[LoRa] Intento JOIN "));
        SerialMon.print(attempt);
        SerialMon.print(F("/"));
        SerialMon.println(maxRetries);

        int16_t state = node->activateOTAA();
        if (state == RADIOLIB_LORAWAN_NEW_SESSION || state == RADIOLIB_LORAWAN_SESSION_RESTORED) {
            SerialMon.println(F("[LoRa] JOIN exitoso"));
            return true;
        }

        SerialMon.print(F("[LoRa] JOIN fallo, codigo "));
        SerialMon.println(state);
        delay(JOIN_RETRY_DELAY_MS);
    }

    return false;
}

LoRaWanTxResult lorawanManagerTransmit(const GpsFix &fix, uint16_t batteryMv)
{
    LoRaWanTxResult result = {false, 0, 0.0f};

    if (node == nullptr) {
        return result;
    }

    uint8_t payload[16];
    size_t length = 0;
    packPayload(fix, batteryMv, payload, length);

    SerialMon.println(F("[LoRa] Enviando uplink..."));
    int16_t state = node->sendReceive(payload, length);

    if (state >= RADIOLIB_ERR_NONE) {
        result.success = true;
        result.rssi = radio->getRSSI();
        result.snr = radio->getSNR();
        SerialMon.print(F("[LoRa] Uplink OK RSSI="));
        SerialMon.print(result.rssi);
        SerialMon.print(F(" SNR="));
        SerialMon.println(result.snr);
    } else {
        SerialMon.print(F("[LoRa] Uplink fallo: "));
        SerialMon.println(state);
    }

    return result;
}

void lorawanManagerSleep()
{
    if (rfPort != nullptr) {
        rfPort->end();
    }

    pinMode(LoRa_Miso, INPUT);
    pinMode(LoRa_Mosi, INPUT);
    pinMode(LoRa_Sclk, INPUT);
    pinMode(LoRa_Cs, INPUT_PULLUP);
    pinMode(LoRa_Rst, INPUT);
    pinMode(LoRa_Dio1, INPUT);
    pinMode(LoRa_Busy, INPUT);
}
