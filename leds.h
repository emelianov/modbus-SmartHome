//////////////////////////////////////////
// ESP8266/ESP32 Modbus SmartHome Device
// (c)2019, a.m.emelianov@gmail.com

#pragma once
#include <Shell.h>
#include <LoginShell.h>
extern LoginShell shell;

#include <NeoPixelBus.h>

const uint16_t PixelCount = 10; // this example assumes 4 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 3;  // make sure to set this to the correct pin, ignored for Esp8266

// These two are the same as above as the DMA method is the default
// NOTE: These will ignore the PIN and use GPI03 (D9) pin
NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(PixelCount, PixelPin);
//NeoPixelBus<NeoRgbFeature, NeoEsp8266Dma400KbpsMethod> strip(PixelCount, PixelPin);

RgbColor useColor(20);

uint32_t ledsInit() {
    strip.Begin();
    strip.SetPixelColor(1, useColor);
    strip.Show();
    return RUN_DELETE;
}

void ledColor(Shell &shell, int argc, const ShellArguments &argv) {
    if (argc > 3) {
        useColor.R = atoi(argv[1]);
        useColor.G = atoi(argv[2]);
        useColor.B = atoi(argv[3]);
    }
    else if (argc == 3) {
      if (argv[1][0] == 'r' || argv[1][0] == 'R') {
        useColor.R = atoi(argv[2]);
      }
      else if (argv[1][0] == 'g' || argv[1][0] == 'G') {
        useColor.G = atoi(argv[2]);
      }
      else if (argv[1][0] == 'b' || argv[1][0] == 'B') {
        useColor.B = atoi(argv[2]);
      }
    }
    shell.printf_P(PSTR("%d %d %d\n"),useColor.R, useColor.G, useColor.B);
}
ShellCommand(ledcolor, "[<R> <G> <B>]|[r <R>]|[g <G>]|[b <B>] - LED Set color to draw", ledColor);

void ledOn(Shell &shell, int argc, const ShellArguments &argv) {
    if (argc > 1) {
        strip.SetPixelColor(atoi(argv[1]), useColor);
        if (argc > 2 && strcmp(argv[2], "show") == 0)
          strip.Show();
    }
}
ShellCommand(ledon, "<pos> [show]- LED Set pixel as position to color (on)", ledOn);

void ledOff(Shell &shell, int argc, const ShellArguments &argv) {
    if (argc > 1) {
        RgbColor black(0);
        strip.SetPixelColor(atoi(argv[1]), black);
        if (argc > 2 && strcmp(argv[2], "show") == 0)
          strip.Show();
    }
}
ShellCommand(ledoff, "<pos> [show] - LED Set pixel as position to black (off)", ledOff);

void ledShow(Shell &shell, int argc, const ShellArguments &argv) {
  strip.Show();
}
ShellCommand(ledshow, " - LED Refresh leds", ledShow);
