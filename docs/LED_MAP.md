# Visual LED map (9×32 on 8×36 wiring)

## Electrical (stock Adalight)

| | |
| --- | --- |
| Strips | **8** on GPIO **15, 14, 13, 12, 11, 10, 9, 8** |
| LEDs / strip | **36** |
| Total | 288 |

Strip 0 = top 9×4 band (GPIO 15) … strip 7 = bottom band (GPIO 8).

## Visual geometry

```text
        x=0                 x=8
          |                   |
        ┌────── 9 cols ───────┐
  y=0   │ strip0 / GPIO15     │  localY 0..3 → LED row starts 0, 8, 17, 26
  y=3   │                     │
        ├─────────────────────┤
  y=4   │ strip1 / GPIO14     │
   …    │         …           │
        ├─────────────────────┤
 y=28   │ strip7 / GPIO8      │
 y=31   └─────────────────────┘
```

## `xyToStripPixel(x, y)`

```text
strip    = y / 4
localY   = y % 4
rowStart = {0, 8, 17, 26}[localY]   // column-0 LEDs on every strip
led      = rowStart + x             // x = 0..8
```

Write: `strips[strip].setPixelColor(led, …)`.

### Column 0 (user sample — matched exactly)

| localY | LED |
| --- | --- |
| 0 | **0** |
| 1 | **8** |
| 2 | **17** |
| 3 | **26** |

Assumption: those indices are **per strip** (same local map on each of the 8 strips). Other columns use `led = rowStart[localY] + x`.

Note: with this chain, LED **8** is both `(x=8, localY=0)` and `(x=0, localY=1)` — the U-turn diode at the end of row 0 / start of row 1. LED **35** is unused. If a future dump shows a clean 0,9,18,27 pitch instead, switch `rowStart` to `{0,9,18,27}`.

## Animation

Crest moves in **visual X** (0..8). Bars span **visual Y** (0..31). Mapping goes through `xyToStripPixel` — not raw strip index as X.
