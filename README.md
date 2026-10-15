# 74HC595 Shift Register Analyzer (Saleae Logic 2)

A custom Low Level Analyzer (LLA) for [Saleae Logic 2](https://www.saleae.com/)
that decodes the serial data clocked into a **74HC595** 8-bit serial-in,
parallel-out shift register and reports each latched output word.

Built against the [Saleae Analyzer SDK](https://github.com/saleae/AnalyzerSDK).

## Why an LLA (not an HLA)?

The 74HC595 needs several signals that must be correlated in time (serial data +
shift clock + latch clock). A High Level Analyzer extension only receives frames
from a single upstream analyzer on one channel, so it can't do this. A Low Level
Analyzer subscribes to multiple raw channels directly, which is what this plugin
does (the same approach the built-in SPI analyzer uses).

## What it decodes

The 74HC595 has two clocks:

| Pin        | Name          | Role                                                        |
|------------|---------------|-------------------------------------------------------------|
| 14         | DS / SER      | Serial data input                                           |
| 11         | SHCP / SRCLK  | Shift register clock — DS is sampled on the **rising** edge |
| 12         | STCP / RCLK   | Storage/latch clock — shift register latched to outputs on **rising** edge |
| 10         | MR / SRCLR    | Master reset (active low) — clears the shift register       |
| 13         | OE            | Output enable (active low)                                  |

On every active edge of **SHCP**, DS is sampled and shifted into an internal
register. On every active edge of **STCP**, the accumulated word is latched and
emitted as a decoded frame. The value is shown on the STCP (latch) channel,
since that is the moment the data appears on the Q outputs.

## Settings

- **DS (Serial Data)** — required. Pin 14.
- **SHCP (Shift Clock)** — required. Pin 11.
- **STCP (Latch Clock)** — required. Pin 12.
- **MR (Reset)** — optional. Pin 10, active low. When it goes low the current
  (partial) word is flagged as `RESET` and the shift register is cleared.
- **OE (Output Enable)** — optional. Pin 13. Reserved for display; does not
  affect decoding.
- **Bits per Word** — number of bits latched per output word. Use `8` for a
  single device, or a multiple of 8 (up to 32) when daisy-chaining `Q7S`/`Q7'`
  of one device into the DS of the next.
- **Shift Clock Active Edge** — rising (74HC595 default) or falling.
- **Latch Clock Active Edge** — rising (74HC595 default) or falling.
- **Bit Order** — `MSB first` (default; the first bit clocked in becomes the
  MSB, matching Arduino `shiftOut(..., MSBFIRST)`) or `LSB first`.

## Output

Each latched word becomes one frame whose value you can display as hex, decimal,
binary, or ASCII. Markers are added for each sampled data bit (1/0), each latch
(up arrow), and each reset (X). The export (CSV/TXT) includes the time, value,
number of bits shifted, and whether a reset occurred.

## Building

Requires CMake 3.13+ and a C++11 compiler. The SDK is fetched automatically.

```bash
cmake -B build -S .
cmake --build build
```

The plugin is produced at:

- macOS:   `build/Analyzers/libHC595Analyzer.so`
- Linux:   `build/Analyzers/libHC595Analyzer.so`
- Windows: `build/Analyzers/HC595Analyzer.dll`

### Loading in Logic 2

1. Open Logic 2 → **Edit menu / preferences** → find the custom analyzer /
   "Custom Low Level Analyzers" directory setting.
2. Point it at the folder containing the built library (e.g. `build/Analyzers`).
3. Restart Logic 2. The analyzer appears as **"74HC595 Shift Register"** in the
   analyzers list.

> On macOS, binaries downloaded from the internet are quarantined and must be
> signed/notarized to load. Locally built binaries (as above) are not affected.

## Testing without hardware

The analyzer ships with a simulation generator. Add the analyzer to a **demo**
device capture in Logic 2 and it will produce a repeating incrementing pattern
clocked out on DS/SHCP and latched on STCP, so you can verify decoding end to end.

## License

Released under the [MIT License](LICENSE). You are free to use, modify, and
distribute this code, including in commercial and closed-source projects, as
long as the copyright notice and license text are retained.

Copyright (c) 2026 Serhii Kulyk
