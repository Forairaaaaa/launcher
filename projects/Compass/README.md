# CardputerZero Compass

Compass app for M5Stack CardputerZero.

## Features

- Show compass heading with animated dial
- Show pitch/roll bubble level overlay
- Expand live accelerometer, gyroscope, and magnetometer values
- Calibrate magnetometer offset/scale with a figure-eight motion
- Save and load calibration data between launches

## Dependencies

Project dependencies are pulled automatically by SCons from `repos.json` when
missing:

- `spdlog`
- `smooth_ui_toolkit`

Image asset conversion runs during build and uses LVGL's Python converter. If
the converter dependencies are missing, install them in your Python environment:

```bash
pip3 install pypng lz4 Pillow
```

Font conversion is not part of the normal build. If fonts are regenerated with
`main/src/assets/convert_fonts.py`, install `lv_font_conv` separately and keep it
available in `PATH`.

## Build

For Linux SDL testing:

```bash
CONFIG_DEFAULT_FILE=linux_x86_sdl2_config_defaults.mk ../../.venv/bin/scons -j8
```

For CardputerZero cross build:

```bash
CONFIG_DEFAULT_FILE=linux_x86_cross_cp0_config_defaults.mk ../../.venv/bin/scons -j8
```

The output binary is `dist/M5CardputerZero-Compass`.

## Usage

Run the SDL build:

```bash
./dist/M5CardputerZero-Compass
```

Calibration data is saved to `calibration.conf` for SDL builds. Packaged
CardputerZero launches set `COMPASS_CONFIG_DIR="$HOME/.config/Compass"`, so
calibration data is saved to `$HOME/.config/Compass/calibration.conf`.

Override the calibration path:

```bash
COMPASS_CALIBRATION_PATH=./calibration.conf ./dist/M5CardputerZero-Compass
```

Key controls:

- Compass page: `8` expand/collapse sensor info, `Esc` exit
- Expanded info: `7` open calibration, `8` collapse sensor info
- Calibration page: `6` or Enter start/save calibration, `4` or Esc back

## Package

Build the CardputerZero Debian package:

```bash
./package_deb.sh
```

The generated package is written to `dist/`:

```text
dist/m5cardputerzero-compass_0.1.0_m5stack1_arm64.deb
```
