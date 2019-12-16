//////////////////////////////////////////
// ESP8266/ESP32 Modbus SmartHome Device
// (c)2019, a.m.emelianov@gmail.com

#pragma once
#include <Shell.h>
#include <LoginShell.h>
extern LoginShell shell;

#include <NeoPixelBus.h>

const uint16_t PixelCount = 4; // this example assumes 4 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266

// These two are the same as above as the DMA method is the default
// NOTE: These will ignore the PIN and use GPI03 (D9) pin
//NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(PixelCount, PixelPin);
NeoPixelBus<NeoRgbFeature, NeoEsp8266Dma400KbpsMethod> strip(PixelCount, PixelPin);

RgbColor useColor(0);

uint32_t ledsInit() {
    strip.Begin();
    strip.Show();
    return RUN_DELETE;
}

void ledColor(Shell &shell, int argc, const ShellArguments &argv) {
    if (argc > 3) {
        useColor.R = atoi(argv[1]);
        useColor.R = atoi(argv[2]);
        useColor.R = atoi(argv[3]);
    }
    shell.printf_P(PSTR("%d %d %d\n"),useColor.R, useColor.G, useColor.B);
}
ShellCommand(ledcolor, "<R> <G> <B> - LED Set color to draw", ledColor);

void ledOn(Shell &shell, int argc, const ShellArguments &argv) {
    if (argc > 1) {
        strip.SetPixelColor(atoi(argv[1]), useColor);
    }
}
ShellCommand(ledon, "<pos> - LED Set pixel as position to color (on)", ledColor);

void ledOff(Shell &shell, int argc, const ShellArguments &argv) {
    if (argc > 1) {
        RgbColor black(0);
        strip.SetPixelColor(atoi(argv[1]), black);
    }
}
ShellCommand(ledoff, "<pos> - LED Set pixel as position to black (off)", ledColor);
