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
#include <math.h>
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
static void xyToStripPixel(uint8_t x, uint8_t y, uint8_t &strip, uint16_t &led);
static void setVisualPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
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

// Map visual (x,y) on the 9×32 panel to (strip, led) on 8×36 wiring.
// Each strip is one 9×4 band; bands stack in Y. See docs/LED_MAP.md.
//
// Column 0 on every strip uses LEDs 0, 8, 17, 26 (user-verified). Other
// columns are the same row starts + x. Row starts are NOT a uniform *9
// pitch (that would be 0,9,18,27); they follow the physical chain folds.
static void xyToStripPixel(uint8_t x, uint8_t y, uint8_t &strip, uint16_t &led) {
  static const uint8_t kRowStart[BAND_ROWS] = {0, 8, 17, 26};
  strip = (uint8_t)(y / BAND_ROWS);
  const uint8_t localY = (uint8_t)(y % BAND_ROWS);
  led = (uint16_t)kRowStart[localY] + x;
}

static void setVisualPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
  uint8_t strip;
  uint16_t led;
  xyToStripPixel(x, y, strip, led);
  if (strip < NUM_STRIPS && led < NUM_LEDS_PER_STRIP) {
    strips[strip].setPixelColor(led, r, g, b);
  }
}

// Pacifica-inspired soft bars on the visual 9×32 map.
// Borrowed from cosmic-unicorn-animartrix pacifica.hpp:
//   • three layered swells at different speeds / spatial freqs
//   • standing-wave Y sway (sin(y)*sin(t)) — morphs in place, no Y-scroll
//   • foam brightening at crests
//   • deep-blue → sea-green → foam palette
// Still: crests travel in visual X; span full visual Y; xyToStripPixel unchanged.
static void renderFlowFrame() {
  const float t = (float)animPhase * 0.045f;
  const float widthF = (float)MATRIX_WIDTH;

  // Three Pacifica-like layers: speed (cols/unit-t), half-width (cols),
  // weight, Y-sway amp (cols), Y spatial freq, sway time freq, phase.
  const float speed[3] = {0.55f, -0.38f, 0.22f};
  const float halfW[3] = {2.1f, 1.6f, 1.35f};  // ~3–5 px full width
  const float weight[3] = {0.50f, 0.32f, 0.22f};
  const float swayAmp[3] = {0.55f, 0.40f, 0.28f};
  const float yFreq[3] = {0.22f, 0.35f, 0.48f};
  const float swayT[3] = {0.70f, 1.15f, 1.60f};
  const float phase[3] = {0.0f, 2.1f, 4.0f};

  for (uint8_t i = 0; i < NUM_STRIPS; i++) {
    strips[i].clear();
  }

  for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
    const float yf = (float)y;
    // Mild static width breathe along height (not time-translated).
    const float widthScale = 1.0f + 0.15f * sinf(yf * 0.20f);

    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
      float depth = 0.0f;
      float foam = 0.0f;

      for (uint8_t L = 0; L < 3; L++) {
        // Standing-wave sway: shape along Y morphs with time, does not scroll.
        const float sway =
            swayAmp[L] * sinf(yf * yFreq[L]) * sinf(t * swayT[L] + phase[L]);
        float crest = fmodf(t * speed[L] * widthF + phase[L] * 1.3f + sway, widthF);
        if (crest < 0.0f) {
          crest += widthF;
        }

        float d = fabsf((float)x + 0.5f - crest);
        const float wrap = widthF - d;
        if (wrap < d) {
          d = wrap;
        }

        const float hw = halfW[L] * widthScale;
        float w = 0.0f;
        if (d < hw) {
          const float u = 1.0f - d / hw;
          w = u * u * (3.0f - 2.0f * u);  // smoothstep
        }
        depth += weight[L] * w;
        // Pacifica foam: lift near the crest peak.
        const float crestLift = w * 0.65f + (L == 0 ? w * 0.20f : 0.0f) - 0.35f;
        if (crestLift > 0.0f) {
          foam += crestLift * 1.8f * weight[L];
        }
      }

      if (depth < 0.02f && foam < 0.02f) {
        continue;
      }
      if (depth > 1.0f) {
        depth = 1.0f;
      }
      if (foam > 1.0f) {
        foam = 1.0f;
      }

      // Deep navy → teal → seafoam (Pacifica stops), then foam whitening.
      float shade = depth * (0.82f + 0.18f * (yf / (float)(MATRIX_HEIGHT - 1)));
      shade += foam * 0.30f;
      if (shade > 1.0f) {
        shade = 1.0f;
      }

      float pr, pg, pb;
      if (shade < 0.25f) {
        const float f = shade / 0.25f;
        pr = 0.02f + (0.00f - 0.02f) * f;
        pg = 0.05f + (0.12f - 0.05f) * f;
        pb = 0.14f + (0.36f - 0.14f) * f;
      } else if (shade < 0.55f) {
        const float f = (shade - 0.25f) / 0.30f;
        pr = 0.00f + (0.02f - 0.00f) * f;
        pg = 0.12f + (0.42f - 0.12f) * f;
        pb = 0.36f + (0.50f - 0.36f) * f;
      } else if (shade < 0.80f) {
        const float f = (shade - 0.55f) / 0.25f;
        pr = 0.02f + (0.20f - 0.02f) * f;
        pg = 0.42f + (0.70f - 0.42f) * f;
        pb = 0.50f + (0.62f - 0.50f) * f;
      } else {
        const float f = (shade - 0.80f) / 0.20f;
        pr = 0.20f + (0.65f - 0.20f) * f;
        pg = 0.70f + (0.90f - 0.70f) * f;
        pb = 0.62f + (0.88f - 0.62f) * f;
      }
      pr += (1.0f - pr) * foam * 0.75f;
      pg += (1.0f - pg) * foam * 0.80f;
      pb += (1.0f - pb) * foam * 0.70f;

      const float bri = 0.50f + 0.50f * shade;
      const uint8_t r = (uint8_t)lroundf(pr * bri * 255.0f);
      const uint8_t g = (uint8_t)lroundf(pg * bri * 255.0f);
      const uint8_t b = (uint8_t)lroundf(pb * bri * 255.0f);
      setVisualPixel(x, y, r, g, b);
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
      replyErr("brightness must be 0-48");
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
    Serial.println(F("Commands: ON | OFF | BRIGHTNESS <0-48> | STATUS | BOOT | HELP"));
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
