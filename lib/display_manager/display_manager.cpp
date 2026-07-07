// Creado por Jonathan Garrido S. - jgarrido.spa@gmail.com
#include "display_manager.h"

#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <HardwarePWM.h>
#include <SPI.h>

#include "config.h"

static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
    GxEPD2_154_D67(ePaper_Cs, ePaper_Dc, ePaper_Rst, ePaper_Busy));

static SPIClass *dispPort = nullptr;
static bool ready = false;

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

static void renderScreen(const char *line1, const char *line2)
{
    if (!ready) {
        return;
    }

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        drawCentered(line1, 70, &FreeMonoBold12pt7b);
        if (line2 != nullptr && line2[0] != '\0') {
            drawCentered(line2, 120, &FreeMono9pt7b);
        }
    } while (display.nextPage());
}

bool displayManagerBegin()
{
    HwPWM0.addPin(ePaper_Backlight);
    HwPWM0.setResolution(8);
    HwPWM0.setClockDiv(PWM_PRESCALER_PRESCALER_DIV_1);
    HwPWM0.begin();
    HwPWM0.writePin(ePaper_Backlight, 80, false);

    dispPort = new SPIClass(NRF_SPIM2, ePaper_Miso, ePaper_Sclk, ePaper_Mosi);
    display.epd2.selectSPI(*dispPort, SPISettings(4000000, MSBFIRST, SPI_MODE0));
    display.init();
    display.setRotation(3);
    ready = true;
    return true;
}

void displayManagerShowBoot()
{
    renderScreen("T-Echo LoRaWAN", "Iniciando...");
}

void displayManagerShowMessage(const char *line1, const char *line2)
{
    renderScreen(line1, line2);
}

void displayManagerShowTransmit(int16_t rssi, float snr)
{
    if (!ready) {
        return;
    }

    char line2[32];
    snprintf(line2, sizeof(line2), "RSSI:%d SNR:%.1f", rssi, snr);

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        drawCentered("TX OK", 60, &FreeMonoBold12pt7b);
        drawCentered(line2, 110, &FreeMono9pt7b);
    } while (display.nextPage());
}

void displayManagerSleep()
{
    if (!ready) {
        return;
    }

    renderScreen("Deep Sleep", nullptr);
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
