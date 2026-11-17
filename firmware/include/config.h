#pragma once

// Framework 16 RGB LED Matrix (Joe Schroedl Rev 2) pinout & layout.
// Stock Adalight firmware documents 8×36 on GPIO 15..8; on-panel inspection
// and the MCU netlist (D7..D15) match a 9×32 grid (still 288 LEDs) with the
// 9th data line on GPIO 7.
// https://github.com/corndog2000/adalight-multi-strip

#include <stdint.h>

static const uint16_t NUM_LEDS_PER_STRIP = 32;
static const uint8_t NUM_STRIPS = 9;
static const uint16_t TOTAL_LEDS = NUM_LEDS_PER_STRIP * NUM_STRIPS;  // 288

// Visual grid: 9 wide × 32 tall.
// Each NeoPixel strip is one vertical COLUMN.
//   X / width  = strip index           (0..8)  — horizontal (crest travels here)
//   Y / height = LED index along strip (0..31) — vertical (bar spans all rows)
// Pixel (x, y) → strips[x].setPixelColor(y, …)
static const uint8_t MATRIX_WIDTH = NUM_STRIPS;
static const uint8_t MATRIX_HEIGHT = NUM_LEDS_PER_STRIP;

// Column 0 → GPIO15 … column 8 → GPIO7 (extends stock 15..8 by one pin).
static const uint8_t STRIP_PINS[NUM_STRIPS] = {15, 14, 13, 12, 11, 10, 9, 8, 7};

// Soft default; hard cap keeps modules within a gentle current budget.
static const uint8_t DEFAULT_BRIGHTNESS = 16;
static const uint8_t MAX_BRIGHTNESS = 48;

static const unsigned long SERIAL_BAUD = 115200;
static const uint16_t FRAME_MS = 33;  // ~30 FPS
