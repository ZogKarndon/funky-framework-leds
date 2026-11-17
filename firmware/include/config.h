#pragma once

// Framework 16 RGB LED Matrix (Joe Schroedl Rev 2) pinout & layout.
// Electrical: 8 NeoPixel strips × 36 LEDs (stock Adalight / GPIO 15..8).
// Visual: 9×32 — eight stacked 9×4 bands (one band per strip).
// https://github.com/corndog2000/adalight-multi-strip

#include <stdint.h>

static const uint16_t NUM_LEDS_PER_STRIP = 36;
static const uint8_t NUM_STRIPS = 8;
static const uint16_t TOTAL_LEDS = NUM_LEDS_PER_STRIP * NUM_STRIPS;  // 288

// Visual panel coordinates (animation space).
static const uint8_t MATRIX_WIDTH = 9;    // columns
static const uint8_t MATRIX_HEIGHT = 32;  // rows (8 bands × 4)
static const uint8_t BAND_ROWS = 4;       // rows per strip / 9×4 block
static const uint8_t BAND_COLS = 9;       // cols per strip / 9×4 block

// Strip data pins: strip 0 (top band) → GPIO15 … strip 7 (bottom) → GPIO8.
static const uint8_t STRIP_PINS[NUM_STRIPS] = {15, 14, 13, 12, 11, 10, 9, 8};

// Soft default; hard cap keeps modules within a gentle current budget.
static const uint8_t DEFAULT_BRIGHTNESS = 16;
static const uint8_t MAX_BRIGHTNESS = 48;

static const unsigned long SERIAL_BAUD = 115200;
static const uint16_t FRAME_MS = 33;  // ~30 FPS
