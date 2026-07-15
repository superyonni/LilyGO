// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
// Basado en: LilyGo-LoRa-Series RadioLib_OTAA + T-Echo Factory/radio.cpp + Meshtastic t-echo
#include "lorawan_manager.h"

#include <RadioLib.h>
#include <SPI.h>

#include "config.h"

static SPIClass *rfPort = nullptr;
static SX1262 *radio = nullptr;
static LoRaWANNode *node = nullptr;
static Module *radioModule = nullptr;

// Patron oficial RadioLib: arrays en .cpp (no static en header)
static uint64_t joinEUI = LORAWAN_JOIN_EUI;
static uint64_t devEUI  = LORAWAN_DEV_EUI;
static uint8_t appKey[] = { LORAWAN_APP_KEY };
// LoRaWAN 1.0.3 OTAA: MIC del JoinRequest usa nwkKey (= AppKey)
static uint8_t nwkKey[] = { LORAWAN_APP_KEY };
#if LORAWAN_USE_ABP
static uint8_t nwkSKey[] = { LORAWAN_NWK_SKEY };
static uint8_t appSKey[] = { LORAWAN_APP_SKEY };
#endif

static void printHexKey(const char *label, const uint8_t *key, size_t len)
{
    SerialMon.print(label);
    for (size_t i = 0; i < len; i++) {
        if (key[i] < 0x10) {
            SerialMon.print('0');
        }
        SerialMon.print(key[i], HEX);
    }
    SerialMon.println();
}

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

static const char *joinErrorText(int16_t code)
{
    switch (code) {
        case RADIOLIB_ERR_NO_JOIN_ACCEPT: return "NO_JOIN_ACCEPT (sin downlink)";
        case RADIOLIB_ERR_RX_TIMEOUT: return "RX_TIMEOUT";
        case RADIOLIB_ERR_DOWNLINK_MALFORMED: return "DOWNLINK_MALFORMED";
        case RADIOLIB_ERR_CRC_MISMATCH: return "CRC_MIC_MISMATCH";
        case RADIOLIB_ERR_JOIN_NONCE_INVALID: return "JOIN_NONCE_INVALID";
        case RADIOLIB_ERR_NO_RX_WINDOW: return "NO_RX_WINDOW";
        default: return "ver codigo RadioLib";
    }
}

bool lorawanManagerBegin()
{
    SerialMon.println(F("[LoRa] Inicializando SX1262 (T-Echo)..."));

    pinMode(Power_Enable_Pin, OUTPUT);
    digitalWrite(Power_Enable_Pin, HIGH);
    delay(10);

    rfPort = new SPIClass(NRF_SPIM3, LoRa_Miso, LoRa_Sclk, LoRa_Mosi);
    rfPort->begin();

    radioModule = new Module(LoRa_Cs, LoRa_Dio1, LoRa_Rst, LoRa_Busy, *rfPort, SPISettings());
    radio = new SX1262(radioModule);

    // Init minimo LilyGO + Meshtastic (no fijar SF/sync aqui; LoRaWANNode lo configura)
    int16_t state = radio->begin();
    if (state != RADIOLIB_ERR_NONE) {
        SerialMon.print(F("[LoRa] begin() fallo: "));
        SerialMon.println(state);
        return false;
    }

    state = radio->setTCXO(1.8);
    if (state != RADIOLIB_ERR_NONE) {
        SerialMon.print(F("[LoRa] setTCXO fallo: "));
        SerialMon.println(state);
        return false;
    }

    state = radio->setDio2AsRfSwitch(true);
    if (state != RADIOLIB_ERR_NONE) {
        SerialMon.print(F("[LoRa] setDio2AsRfSwitch fallo: "));
        SerialMon.println(state);
        return false;
    }

    radio->setOutputPower(10);
    radio->setCurrentLimit(140);
    radio->setRxBoostedGainMode(true);

    node = new LoRaWANNode(radio, &AU915, LORAWAN_SUBBAND);

#if LORAWAN_USE_ABP
    int16_t abpState = node->beginABP(LORAWAN_DEV_ADDR, nullptr, nullptr, nwkSKey, appSKey);
    if (abpState != RADIOLIB_ERR_NONE) {
        SerialMon.print(F("[LoRa] beginABP fallo: "));
        SerialMon.println(abpState);
        return false;
    }
    node->setADR(true);
    node->setDwellTime(true, 400);
    SerialMon.println(F("[LoRa] Modo ABP"));
    SerialMon.print(F("[LoRa] DevAddr=0x"));
    SerialMon.println((unsigned long)LORAWAN_DEV_ADDR, HEX);
    printHexKey("[LoRa] NwkSKey=", nwkSKey, 16);
    printHexKey("[LoRa] AppSKey=", appSKey, 16);
#else
    node->beginOTAA(joinEUI, devEUI, nwkKey, appKey);
    node->setADR(false);
    node->setDwellTime(true, 400);
    SerialMon.println(F("[LoRa] Modo OTAA"));
    SerialMon.print(F("[LoRa] DevEUI="));
    for (int i = 7; i >= 0; i--) {
        uint8_t b = (uint8_t)((devEUI >> (i * 8)) & 0xFF);
        if (b < 0x10) SerialMon.print('0');
        SerialMon.print(b, HEX);
    }
    SerialMon.println();
    printHexKey("[LoRa] AppKey=", appKey, 16);
#endif
    SerialMon.print(F("[LoRa] Subbanda AU915="));
    SerialMon.println(LORAWAN_SUBBAND);

    return true;
}

bool lorawanManagerJoin(uint8_t maxRetries)
{
    if (node == nullptr) {
        return false;
    }

#if LORAWAN_USE_ABP
    SerialMon.println(F("[LoRa] Activando ABP (sin JoinRequest)..."));
    int16_t state = node->activateABP(2);
    if (state == RADIOLIB_LORAWAN_NEW_SESSION || state == RADIOLIB_LORAWAN_SESSION_RESTORED
        || state == RADIOLIB_ERR_NONE) {
        SerialMon.print(F("[LoRa] ABP activo DevAddr=0x"));
        SerialMon.println((unsigned long)node->getDevAddr(), HEX);
        return true;
    }
    SerialMon.print(F("[LoRa] activateABP fallo: "));
    SerialMon.println(state);
    return false;
#else
    for (uint8_t attempt = 1; attempt <= maxRetries; attempt++) {
        SerialMon.print(F("[LoRa] Intento JOIN "));
        SerialMon.print(attempt);
        SerialMon.print(F("/"));
        SerialMon.println(maxRetries);

        // AU915: DR2=SF10 (dwell OK). Si falla, probar DR3 en reintento par.
        uint8_t joinDr = (attempt % 2 == 0) ? 3 : 2;
        SerialMon.print(F("[LoRa] Join DR"));
        SerialMon.println(joinDr);

        int16_t state = node->activateOTAA(joinDr);
        if (state == RADIOLIB_LORAWAN_NEW_SESSION || state == RADIOLIB_LORAWAN_SESSION_RESTORED) {
            node->setADR(true);
            SerialMon.print(F("[LoRa] JOIN exitoso DevAddr=0x"));
            SerialMon.println((unsigned long)node->getDevAddr(), HEX);
            return true;
        }

        SerialMon.print(F("[LoRa] JOIN fallo codigo "));
        SerialMon.print(state);
        SerialMon.print(F(" ("));
        SerialMon.print(joinErrorText(state));
        SerialMon.println(F(")"));
        if (state == RADIOLIB_ERR_NO_JOIN_ACCEPT) {
            SerialMon.println(F("[LoRa] -> Gateway: ¿JoinAccept tras JoinRequest?"));
            SerialMon.println(F("[LoRa] -> ChirpStack: Events/LoRaWAN frames techo-01"));
            SerialMon.println(F("[LoRa] -> Si NO hay JoinAccept: AppKey o OTAA keys"));
            SerialMon.println(F("[LoRa] -> Si SI hay JoinAccept: problema RX downlink"));
        }
        delay(JOIN_RETRY_DELAY_MS);
    }

    return false;
#endif
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

    SerialMon.print(F("[LoRa] Payload hex: "));
    for (size_t i = 0; i < length; i++) {
        if (payload[i] < 0x10) {
            SerialMon.print('0');
        }
        SerialMon.print(payload[i], HEX);
    }
    SerialMon.println();
    if (fix.valid) {
        SerialMon.print(F("[LoRa] GPS lat="));
        SerialMon.print(fix.latitude, 6);
        SerialMon.print(F(" lon="));
        SerialMon.println(fix.longitude, 6);
    }

    SerialMon.println(F("[LoRa] Enviando uplink FPort 1..."));
    SerialMon.print(F("[LoRa] FCntUp="));
    SerialMon.println(node->getFCntUp() + 1);

    int16_t state = node->sendReceive(payload, length, 1);

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

uint32_t lorawanManagerGetFCntUp()
{
    if (node == nullptr) {
        return 0;
    }
    return node->getFCntUp();
}

uint32_t lorawanManagerGetDevAddr()
{
    if (node == nullptr) {
        return LORAWAN_DEV_ADDR;
    }
    return node->getDevAddr();
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
