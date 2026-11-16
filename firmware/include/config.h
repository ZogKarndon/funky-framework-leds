#pragma once

// Framework 16 RGB LED Matrix (Joe Schroedl Rev 2) pinout & layout.
// Source of truth for stock Adalight firmware:
// https://github.com/corndog2000/adalight-multi-strip

#include <stdint.h>

static const uint16_t NUM_LEDS_PER_STRIP = 36;
static const uint8_t NUM_STRIPS = 8;
static const uint16_t TOTAL_LEDS = NUM_LEDS_PER_STRIP * NUM_STRIPS;

// Columns = strips; rows = LEDs along each strip.
static const uint8_t MATRIX_WIDTH = NUM_STRIPS;
static const uint8_t MATRIX_HEIGHT = NUM_LEDS_PER_STRIP;

static const uint8_t STRIP_PINS[NUM_STRIPS] = {15, 14, 13, 12, 11, 10, 9, 8};

// Keep default modest so a full-panel pattern stays near the input-module
// ~500 mA budget. Host can raise brightness intentionally.
static const uint8_t DEFAULT_BRIGHTNESS = 40;
static const uint8_t MAX_BRIGHTNESS = 255;

static const unsigned long SERIAL_BAUD = 115200;
static const uint16_t FRAME_MS = 33;  // ~30 FPS flowing animation
