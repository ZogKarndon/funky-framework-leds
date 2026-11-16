/*
 * funky-framework-leds — stand-alone flowing animation for Framework 16
 * RGB LED Matrix modules (Joe Schroedl Rev 2 / NeoPixel on RP2040).
 *
 * Animation runs on-device without a host. USB CDC serial accepts power
 * and brightness commands (see docs/PROTOCOL.md).
 *
 * Build/flash with PlatformIO (see firmware/platformio.ini and README).
 */

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <stdlib.h>
#include <strings.h>
#include "pico/bootrom.h"
#include "config.h"

#define NEO_TYPE (NEO_GRB + NEO_KHZ800)

Adafruit_NeoPixel strips[NUM_STRIPS] = {
  Adafruit_NeoPixel(NUM_LEDS_PER_STRIP, STRIP_PINS[0], NEO_TYPE),
  Adafruit_NeoPixel(NUM_LEDS_PER_STRIP, STRIP_PINS[1], NEO_TYPE),
  Adafruit_NeoPixel(NUM_LEDS_PER_STRIP, STRIP_PINS[2], NEO_TYPE),
  Adafruit_NeoPixel(NUM_LEDS_PER_STRIP, STRIP_PINS[3], NEO_TYPE),
  Adafruit_NeoPixel(NUM_LEDS_PER_STRIP, STRIP_PINS[4], NEO_TYPE),
  Adafruit_NeoPixel(NUM_LEDS_PER_STRIP, STRIP_PINS[5], NEO_TYPE),
  Adafruit_NeoPixel(NUM_LEDS_PER_STRIP, STRIP_PINS[6], NEO_TYPE),
  Adafruit_NeoPixel(NUM_LEDS_PER_STRIP, STRIP_PINS[7], NEO_TYPE),
};

static bool powerOn = true;
static uint8_t brightness = DEFAULT_BRIGHTNESS;
static uint16_t animPhase = 0;
static unsigned long lastFrameMs = 0;

static char lineBuf[64];
static uint8_t lineLen = 0;

static void applyBrightness();
static void clearAll();
static void showAll();
static void renderFlowFrame();
static void wheel(uint8_t pos, uint8_t &r, uint8_t &g, uint8_t &b);
static void handleLine(char *line);
static void processSerial();
static void replyOk();
static void replyStatus();
static void replyErr(const char *msg);

void setup() {
  for (uint8_t i = 0; i < NUM_STRIPS; i++) {
    strips[i].begin();
    strips[i].clear();
  }
  applyBrightness();
  showAll();

  Serial.begin(SERIAL_BAUD);
  Serial.println(F("FUNKY FW16 RGB ready"));
  replyStatus();
  lastFrameMs = millis();
}

void loop() {
  processSerial();

  const unsigned long now = millis();
  if ((uint16_t)(now - lastFrameMs) >= FRAME_MS) {
    lastFrameMs = now;
    if (powerOn) {
      renderFlowFrame();
      animPhase++;
    }
  }
}

static void applyBrightness() {
  for (uint8_t i = 0; i < NUM_STRIPS; i++) {
    strips[i].setBrightness(brightness);
  }
}

static void clearAll() {
  for (uint8_t i = 0; i < NUM_STRIPS; i++) {
    strips[i].clear();
  }
}

static void showAll() {
  for (uint8_t i = 0; i < NUM_STRIPS; i++) {
    strips[i].show();
  }
}

static void wheel(uint8_t pos, uint8_t &r, uint8_t &g, uint8_t &b) {
  pos = 255 - pos;
  if (pos < 85) {
    r = 255 - pos * 3;
    g = 0;
    b = pos * 3;
  } else if (pos < 170) {
    pos -= 85;
    r = 0;
    g = pos * 3;
    b = 255 - pos * 3;
  } else {
    pos -= 170;
    r = pos * 3;
    g = 255 - pos * 3;
    b = 0;
  }
}

// Vertical wave: a soft color crest travels along panel height (Y).
// All columns share the envelope (horizontal band moving up/down the 8×36
// matrix). Hue shifts slowly over time.
static void renderFlowFrame() {
  const uint8_t crest = (uint8_t)(animPhase & 0xFF);
  const uint8_t baseHue = (uint8_t)((animPhase / 2) & 0xFF);

  for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
    const uint8_t yPhase =
        (uint8_t)((uint16_t)y * 255 / (MATRIX_HEIGHT - 1));
    uint8_t dist = (uint8_t)(yPhase - crest);
    if (dist > 128) {
      dist = (uint8_t)(256 - dist);  // circular distance on the strip
    }
    // Soft band ~±32 phase units wide; elsewhere off.
    const uint8_t level =
        dist < 32 ? (uint8_t)(255 - (uint16_t)dist * 255 / 32) : 0;

    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
      uint8_t r, g, b;
      wheel((uint8_t)(baseHue + x * 8), r, g, b);
      r = (uint8_t)((uint16_t)r * level / 255);
      g = (uint8_t)((uint16_t)g * level / 255);
      b = (uint8_t)((uint16_t)b * level / 255);
      strips[x].setPixelColor(y, r, g, b);
    }
  }
  showAll();
}

static void replyOk() {
  Serial.println(F("OK"));
}

static void replyStatus() {
  Serial.print(F("STATUS power="));
  Serial.print(powerOn ? F("on") : F("off"));
  Serial.print(F(" brightness="));
  Serial.println(brightness);
}

static void replyErr(const char *msg) {
  Serial.print(F("ERR "));
  Serial.println(msg);
}

static void handleLine(char *line) {
  while (*line == ' ' || *line == '\t') {
    line++;
  }
  size_t n = strlen(line);
  while (n > 0 && (line[n - 1] == ' ' || line[n - 1] == '\t' ||
                   line[n - 1] == '\r')) {
    line[--n] = '\0';
  }
  if (n == 0) {
    return;
  }

  if (strcasecmp(line, "ON") == 0) {
    powerOn = true;
    replyOk();
    replyStatus();
    return;
  }
  if (strcasecmp(line, "OFF") == 0) {
    powerOn = false;
    clearAll();
    showAll();
    replyOk();
    replyStatus();
    return;
  }
  if (strcasecmp(line, "STATUS") == 0 || strcasecmp(line, "GET") == 0) {
    replyStatus();
    return;
  }
  if (strcasecmp(line, "BOOT") == 0 || strcasecmp(line, "BOOTLOADER") == 0) {
    replyOk();
    Serial.flush();
    delay(50);
    reset_usb_boot(0, 0);
    return;
  }
  if (strncasecmp(line, "BRIGHTNESS ", 11) == 0 ||
      strncasecmp(line, "BRIGHT ", 7) == 0) {
    const char *arg =
        (strncasecmp(line, "BRIGHTNESS ", 11) == 0) ? line + 11 : line + 7;
    while (*arg == ' ') {
      arg++;
    }
    char *end = nullptr;
    long v = strtol(arg, &end, 10);
    if (end == arg || v < 0 || v > MAX_BRIGHTNESS) {
      replyErr("brightness must be 0-255");
      return;
    }
    brightness = (uint8_t)v;
    applyBrightness();
    if (!powerOn) {
      clearAll();
      showAll();
    }
    replyOk();
    replyStatus();
    return;
  }
  if (strcasecmp(line, "HELP") == 0) {
    Serial.println(F("Commands: ON | OFF | BRIGHTNESS <0-255> | STATUS | BOOT | HELP"));
    return;
  }

  replyErr("unknown command (try HELP)");
}

static void processSerial() {
  while (Serial.available() > 0) {
    const char c = (char)Serial.read();
    if (c == '\n') {
      lineBuf[lineLen] = '\0';
      handleLine(lineBuf);
      lineLen = 0;
    } else if (c == '\r') {
      // ignore
    } else if (lineLen + 1 < sizeof(lineBuf)) {
      lineBuf[lineLen++] = c;
    } else {
      lineLen = 0;
      replyErr("line too long");
      while (Serial.available() > 0 && Serial.peek() != '\n') {
        Serial.read();
      }
    }
  }
}
