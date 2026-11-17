#pragma once

// Framework 16 RGB LED Matrix (Joe Schroedl Rev 2) pinout & layout.
// Source of truth for stock Adalight firmware:
// https://github.com/corndog2000/adalight-multi-strip

#include <stdint.h>

static const uint16_t NUM_LEDS_PER_STRIP = 36;
static const uint8_t NUM_STRIPS = 8;
static const uint16_t TOTAL_LEDS = NUM_LEDS_PER_STRIP * NUM_STRIPS;

// Visual grid (on-device, after #5/#6 flash reports): 36 wide × 8 tall.
// Each NeoPixel strip is one horizontal ROW — strip index is vertical.
// Traveling in strip index scrolled top↔bottom on the laptop; traveling
// along the LED index moves left↔right.
//   X / width  = LED index along strip (0..35) — horizontal
//   Y / height = strip index           (0..7)  — vertical
// Pixel (x, y) → strips[y].setPixelColor(x, …)
static const uint8_t MATRIX_WIDTH = NUM_LEDS_PER_STRIP;
static const uint8_t MATRIX_HEIGHT = NUM_STRIPS;

static const uint8_t STRIP_PINS[NUM_STRIPS] = {15, 14, 13, 12, 11, 10, 9, 8};

// Soft default; hard cap keeps modules within a gentle current budget.
static const uint8_t DEFAULT_BRIGHTNESS = 16;
static const uint8_t MAX_BRIGHTNESS = 48;

static const unsigned long SERIAL_BAUD = 115200;
static const uint16_t FRAME_MS = 33;  // ~30 FPS

