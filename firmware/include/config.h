#pragma once

// Framework 16 RGB LED Matrix (Joe Schroedl Rev 2) pinout & layout.
// Source of truth for stock Adalight firmware:
// https://github.com/corndog2000/adalight-multi-strip

#include <stdint.h>

static const uint16_t NUM_LEDS_PER_STRIP = 36;
static const uint8_t NUM_STRIPS = 8;
static const uint16_t TOTAL_LEDS = NUM_LEDS_PER_STRIP * NUM_STRIPS;

// Visual grid: 8 wide × 36 tall (Framework portrait / Joe Rev 2).
// Each NeoPixel strip is one vertical COLUMN.
//   X / width  = strip index           (0..7)  — horizontal (crest travels here)
//   Y / height = LED index along strip (0..35) — vertical (bar spans all rows)
// Pixel (x, y) → strips[x].setPixelColor(y, …)
static const uint8_t MATRIX_WIDTH = NUM_STRIPS;
static const uint8_t MATRIX_HEIGHT = NUM_LEDS_PER_STRIP;

static const uint8_t STRIP_PINS[NUM_STRIPS] = {15, 14, 13, 12, 11, 10, 9, 8};

// Soft default; hard cap keeps modules within a gentle current budget.
static const uint8_t DEFAULT_BRIGHTNESS = 16;
static const uint8_t MAX_BRIGHTNESS = 48;

static const unsigned long SERIAL_BAUD = 115200;
static const uint16_t FRAME_MS = 33;  // ~30 FPS

