# Firmware (`funky_leds`)

Arduino sketch for Framework 16 RGB LED Matrix modules (8×36 NeoPixels on RP2040).

- Starts a flowing diagonal rainbow immediately on boot (no host required).
- Listens on USB CDC @ 115200 for `ON` / `OFF` / `BRIGHTNESS` / `STATUS` / `BOOT`.
- Pin map matches [adalight-multi-strip](https://github.com/corndog2000/adalight-multi-strip).

See the root [README.md](../README.md) for flash instructions and [docs/PROTOCOL.md](../docs/PROTOCOL.md) for the serial API.
