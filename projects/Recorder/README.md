# CardputerZero Recorder

Audio recorder app for M5Stack CardputerZero.

## Features

- Record WAV audio with live waveform and spectrum views
- Pause and resume while recording
- Confirm, rename, or discard a recording after stopping
- Browse and delete saved recordings
- Play recordings with seek and 1x/2x/5x speed controls

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

The output binary is `dist/M5CardputerZero-Recorder`.

## Usage

Run with the default `recordings/` directory:

```bash
./dist/M5CardputerZero-Recorder
```

Use a custom recordings directory:

```bash
./dist/M5CardputerZero-Recorder --recordings-dir ./recordings
```

Key controls:

- Recording page: `6` start/stop, `5` pause/resume, `4` waveform mode, `8` files, `Esc` exit
- Files page: `5`/`6` or Up/Down select, `7` or Enter play, `8` delete, `4` or Esc back
- Playback page: `5` back 10s, `6` play/pause, `7` forward 10s, `8` speed, `4` or Esc back
