// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#include "display_manager.h"

#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <HardwarePWM.h>
#include <SPI.h>

#include "config.h"
#include "gps_manager.h"

static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
    GxEPD2_154_D67(ePaper_Cs, ePaper_Dc, ePaper_Rst, ePaper_Busy));

static SPIClass *dispPort = nullptr;
static bool ready = false;
static bool partialReady = false;
static bool userBrowsing = false;
static DisplayPage currentPage = DisplayPage::OVERVIEW;
static DisplaySnapshot snap = {};
static double mapRefLat = 0.0;
static double mapRefLon = 0.0;
static bool mapRefSet = false;

static void drawCentered(const char *text, int16_t y, const GFXfont *font)
{
    display.setFont(font);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
    int16_t x = ((display.width() - tbw) / 2) - tbx;
    display.setCursor(x, y);
    display.print(text);
}

static void drawLeft(const char *text, int16_t x, int16_t y, const GFXfont *font)
{
    display.setFont(font);
    display.setCursor(x, y);
    display.print(text);
}

static void drawHeader(const char *subtitle)
{
    drawLeft("WISENSOR", 4, 18, &FreeMonoBold12pt7b);
    if (subtitle != nullptr) {
        drawLeft(subtitle, 4, 34, &FreeMono9pt7b);
    }
}

static void drawFooter()
{
    char footer[32];
    snprintf(footer, sizeof(footer), "%u/%u clic=menu hold=TX",
        (unsigned)(currentPage) + 1, (unsigned)DisplayPage::COUNT);
    drawLeft(footer, 4, display.height() - 4, &FreeMono9pt7b);
}

static const char *rssiLabel(int16_t rssi)
{
    if (rssi >= RSSI_EXCELLENT_DBM) {
        return "EXCEL";
    }
    if (rssi >= RSSI_GOOD_DBM) {
        return "BUENO";
    }
    if (rssi >= RSSI_WEAK_DBM) {
        return "DEBIL";
    }
    return "MALO";
}

static const char *snrLabel(float snr)
{
    if (snr >= SNR_EXCELLENT_DB) {
        return "EXCEL";
    }
    if (snr >= SNR_GOOD_DB) {
        return "BUENO";
    }
    if (snr >= SNR_WEAK_DB) {
        return "DEBIL";
    }
    return "MALO";
}

static void updateMapReference()
{
    if (!mapRefSet && snap.fix.valid) {
        mapRefLat = snap.fix.latitude;
        mapRefLon = snap.fix.longitude;
        mapRefSet = true;
    }
}

static void drawMiniMap(int16_t ox, int16_t oy, int16_t w, int16_t h)
{
    display.drawRect(ox, oy, w, h, GxEPD_BLACK);
    display.drawLine(ox + w / 2, oy, ox + w / 2, oy + h, GxEPD_BLACK);
    display.drawLine(ox, oy + h / 2, ox + w, oy + h / 2, GxEPD_BLACK);

    // Marca de referencia (centro / primer fix)
    display.drawCircle(ox + w / 2, oy + h / 2, 5, GxEPD_BLACK);

    if (!snap.fix.valid || !mapRefSet) {
        drawLeft("Sin GPS", ox + w / 2 - 28, oy + h / 2 + 18, &FreeMono9pt7b);
        return;
    }

    double dx = (snap.fix.longitude - mapRefLon) / MAP_SPAN_DEG;
    double dy = (mapRefLat - snap.fix.latitude) / MAP_SPAN_DEG;

    if (dx > 1.0) dx = 1.0;
    if (dx < -1.0) dx = -1.0;
    if (dy > 1.0) dy = 1.0;
    if (dy < -1.0) dy = -1.0;

    int16_t px = ox + w / 2 + (int16_t)(dx * (w / 2 - 6));
    int16_t py = oy + h / 2 + (int16_t)(dy * (h / 2 - 6));
    display.fillCircle(px, py, 5, GxEPD_BLACK);
    display.drawLine(px - 7, py, px + 7, py, GxEPD_BLACK);
    display.drawLine(px, py - 7, px, py + 7, GxEPD_BLACK);
}

static void beginDraw(bool fullRefresh)
{
    if (!fullRefresh && partialReady && display.epd2.hasPartialUpdate) {
        display.setPartialWindow(0, 0, display.width(), display.height());
    } else {
        display.setFullWindow();
    }
}

static void renderPage()
{
    if (!ready) {
        return;
    }

    updateMapReference();

    char buf[48];
    float batV = snap.batteryMv / 1000.0f;

    beginDraw(false);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        switch (currentPage) {
            case DisplayPage::OVERVIEW:
                drawHeader("Resumen");
                snprintf(buf, sizeof(buf), "%02u:%02u:%02u", snap.hour, snap.minute, snap.second);
                drawLeft(buf, 130, 18, &FreeMono9pt7b);
                drawLeft(snap.txOk ? "TX: OK" : "TX: FAIL", 4, 58, &FreeMonoBold12pt7b);
                snprintf(buf, sizeof(buf), "Red: %s", snap.networkOk ? "ABP OK" : "OFF");
                drawLeft(buf, 4, 82, &FreeMono9pt7b);
                snprintf(buf, sizeof(buf), "Bat: %.2f V", batV);
                drawLeft(buf, 4, 100, &FreeMono9pt7b);
                snprintf(buf, sizeof(buf), "GPS: %s  S:%u",
                    snap.fix.valid ? "FIX" : "NO", snap.fix.satellites);
                drawLeft(buf, 4, 118, &FreeMono9pt7b);
                if (snap.txOk) {
                    snprintf(buf, sizeof(buf), "RSSI %d [%s]", snap.rssi, rssiLabel(snap.rssi));
                    drawLeft(buf, 4, 136, &FreeMono9pt7b);
                    snprintf(buf, sizeof(buf), "SNR %.1f [%s]", snap.snr, snrLabel(snap.snr));
                    drawLeft(buf, 4, 154, &FreeMono9pt7b);
                }
                drawLeft("AST / techo-01", 4, 172, &FreeMono9pt7b);
                break;

            case DisplayPage::GPS_DATA:
                drawHeader("GPS");
                if (snap.fix.valid) {
                    snprintf(buf, sizeof(buf), "Lat: %.5f", snap.fix.latitude);
                    drawLeft(buf, 4, 60, &FreeMono9pt7b);
                    snprintf(buf, sizeof(buf), "Lon: %.5f", snap.fix.longitude);
                    drawLeft(buf, 4, 78, &FreeMono9pt7b);
                    snprintf(buf, sizeof(buf), "Alt: %.0f m", snap.fix.altitude);
                    drawLeft(buf, 4, 96, &FreeMono9pt7b);
                    snprintf(buf, sizeof(buf), "Sats: %u", snap.fix.satellites);
                    drawLeft(buf, 4, 114, &FreeMono9pt7b);
                    drawLeft("Estado: FIX OK", 4, 140, &FreeMono9pt7b);
                } else {
                    drawLeft("Sin fix valido", 4, 70, &FreeMonoBold12pt7b);
                    snprintf(buf, sizeof(buf), "Sats visibles: %u", snap.fix.satellites);
                    drawLeft(buf, 4, 100, &FreeMono9pt7b);
                    drawLeft("Sal al aire libre", 4, 130, &FreeMono9pt7b);
                }
                break;

            case DisplayPage::LORA_RF:
                drawHeader("LoRa RF");
                snprintf(buf, sizeof(buf), "Addr: %08lX", (unsigned long)snap.devAddr);
                drawLeft(buf, 4, 58, &FreeMono9pt7b);
                snprintf(buf, sizeof(buf), "FCnt: %lu", (unsigned long)snap.fCnt);
                drawLeft(buf, 4, 76, &FreeMono9pt7b);
                snprintf(buf, sizeof(buf), "RSSI: %d dBm", snap.rssi);
                drawLeft(buf, 4, 100, &FreeMono9pt7b);
                snprintf(buf, sizeof(buf), "  -> %s (>%d)",
                    rssiLabel(snap.rssi), RSSI_GOOD_DBM);
                drawLeft(buf, 4, 116, &FreeMono9pt7b);
                snprintf(buf, sizeof(buf), "SNR: %.1f dB", snap.snr);
                drawLeft(buf, 4, 140, &FreeMono9pt7b);
                snprintf(buf, sizeof(buf), "  -> %s (>%d)",
                    snrLabel(snap.snr), (int)SNR_GOOD_DB);
                drawLeft(buf, 4, 156, &FreeMono9pt7b);
                drawLeft("Umbrales en config.h", 4, 176, &FreeMono9pt7b);
                break;

            case DisplayPage::MAP_VIEW:
                drawHeader("Mapa");
                drawMiniMap(12, 42, 176, 110);
                if (snap.fix.valid) {
                    snprintf(buf, sizeof(buf), "FIX S:%u", snap.fix.satellites);
                    drawLeft(buf, 4, 162, &FreeMono9pt7b);
                    snprintf(buf, sizeof(buf), "%.5f %.5f", snap.fix.latitude, snap.fix.longitude);
                    drawLeft(buf, 4, 178, &FreeMono9pt7b);
                } else {
                    drawLeft("Esperando GPS...", 4, 170, &FreeMono9pt7b);
                }
                break;

            default:
                break;
        }

        drawFooter();
    } while (display.nextPage());

    partialReady = true;
}

bool displayManagerBegin()
{
    pinMode(Power_Enable_Pin, OUTPUT);
    digitalWrite(Power_Enable_Pin, HIGH);
    delay(10);

    HwPWM0.addPin(ePaper_Backlight);
    HwPWM0.setResolution(8);
    HwPWM0.setClockDiv(PWM_PRESCALER_PRESCALER_DIV_1);
    HwPWM0.begin();
    HwPWM0.writePin(ePaper_Backlight, 140, false);

    dispPort = new SPIClass(NRF_SPIM2, ePaper_Miso, ePaper_Sclk, ePaper_Mosi);
    display.epd2.selectSPI(*dispPort, SPISettings(4000000, MSBFIRST, SPI_MODE0));
    display.init(0, true, 50, false);
    display.setRotation(3);
    ready = true;
    return true;
}

void displayManagerShowBootAnimation()
{
    if (!ready) {
        return;
    }

    beginDraw(true);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        drawCentered("AST", 58, &FreeMonoBold24pt7b);
        drawCentered("WISENSOR", 100, &FreeMonoBold12pt7b);
        drawCentered("T-Echo LoRaWAN", display.height() - 18, &FreeMono9pt7b);
    } while (display.nextPage());

    partialReady = true;
}

void displayManagerShowMessage(const char *line1, const char *line2)
{
    if (!ready) {
        return;
    }

    beginDraw(!partialReady);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        drawHeader("WISENSOR");
        drawCentered(line1, 90, &FreeMonoBold12pt7b);
        if (line2 != nullptr && line2[0] != '\0') {
            drawCentered(line2, 125, &FreeMono9pt7b);
        }
    } while (display.nextPage());

    partialReady = true;
}

void displayManagerShowGpsProgress(uint8_t satellites, uint32_t elapsedSec)
{
    if (!ready || userBrowsing) {
        return;
    }

    char line1[28];
    char line2[28];
    snprintf(line1, sizeof(line1), "Tiempo: %lus", (unsigned long)elapsedSec);
    snprintf(line2, sizeof(line2), "Satelites: %u", satellites);

    beginDraw(false);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        drawHeader("GPS");
        drawCentered("Buscando...", 80, &FreeMonoBold12pt7b);
        drawCentered(line1, 115, &FreeMono9pt7b);
        drawCentered(line2, 140, &FreeMono9pt7b);
        drawLeft("Clic=menu  Hold=TX", 4, display.height() - 4, &FreeMono9pt7b);
    } while (display.nextPage());
}

void displayManagerSetSnapshot(const DisplaySnapshot &data)
{
    snap = data;
}

void displayManagerSetPage(DisplayPage page)
{
    if (page < DisplayPage::COUNT) {
        currentPage = page;
    }
}

DisplayPage displayManagerGetPage()
{
    return currentPage;
}

DisplayPage displayManagerNextPage()
{
    uint8_t next = ((uint8_t)currentPage + 1) % (uint8_t)DisplayPage::COUNT;
    currentPage = (DisplayPage)next;
    return currentPage;
}

void displayManagerRenderCurrentPage()
{
    renderPage();
}

void displayManagerSetUserBrowsing(bool browsing)
{
    userBrowsing = browsing;
}

bool displayManagerIsUserBrowsing()
{
    return userBrowsing;
}

void displayManagerSleep()
{
    if (!ready) {
        return;
    }

    displayManagerShowMessage("Deep Sleep", "WISENSOR");
    HwPWM0.writePin(ePaper_Backlight, 0, false);

    if (dispPort != nullptr) {
        dispPort->end();
    }

    pinMode(ePaper_Miso, INPUT);
    pinMode(ePaper_Mosi, INPUT);
    pinMode(ePaper_Sclk, INPUT);
    pinMode(ePaper_Cs, INPUT);
    pinMode(ePaper_Dc, INPUT);
    pinMode(ePaper_Rst, INPUT);
    pinMode(ePaper_Busy, INPUT);
}
