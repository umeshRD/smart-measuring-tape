# Smart Measuring Tape

A pocket-sized digital measuring-device prototype for straight, curved, and round surfaces, developed and presented at **WAYAINNOFEST 2025, Wayamba University of Sri Lanka**.

![Smart measuring tape prototype](1754930576415.jfif)

## Overview

The firmware converts rotary-encoder pulses into distance and displays the result on a 128 × 64 OLED. It includes unit selection, calibration storage, and a battery-level display. The original project description places this work within an IoT concept; the checked-in sketch should be reviewed separately for any required connectivity features.

## Firmware features

- Rotary-encoder distance measurement.
- Display units in centimetres, metres, and feet.
- Reset and unit-selection buttons.
- Saved distance-per-pulse calibration using `Preferences`.
- SSD1306 OLED display at I2C address `0x3C`.
- Battery-voltage measurement through a resistor divider.

## Pin assignments

| Component | Firmware pin |
| --- | --- |
| Encoder CLK / DT | 4 / 5 |
| Reset button | 9 |
| Unit button | 10 |
| Battery ADC | 3 |
| OLED SDA / SCL | 6 / 7 |

The sketch uses ESP32-specific APIs, including `Preferences`. Confirm the exact board, usable pins, voltage levels, and wiring against your hardware before uploading.

## Getting started

1. Download the repository and open [`final.ino`](final.ino) in Arduino IDE. If prompted, place it in a folder named `final`.
2. Install the appropriate ESP32 board package for your hardware.
3. Install **Adafruit GFX Library** and **Adafruit SSD1306** through Library Manager. `Wire` and `Preferences` are supplied by the relevant board/core libraries.
4. Select the board and serial port, review the pin definitions, then compile and upload.
5. Open Serial Monitor at 115200 baud and verify the OLED, buttons, and encoder.
6. Calibrate against a known distance before relying on displayed measurements.

## Repository guide

- [`final.ino`](final.ino) — measuring-device firmware.
- [`Smart_Measuring_Tape_Proposal.pdf`](Smart_Measuring_Tape_Proposal.pdf) — project proposal.
- [`Blink.txt`](Blink.txt) — supplementary sketch material.
- `*.jfif` — project photographs.

## Project status

This repository presents a prototype, its firmware, and supporting material. Measurement accuracy, board compatibility, and battery calibration require hardware validation; no measured accuracy results are included here.

## Author

[Umesh R.D.S](https://github.com/umeshRD) — Electrotechnology undergraduate, Wayamba University of Sri Lanka.
