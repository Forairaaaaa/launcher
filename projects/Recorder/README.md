# CardputerZero Recorder

Audio recorder app for M5Stack CardputerZero.

## Features

- Record WAV audio with live waveform and spectrum views
- Pause and resume while recording
- Confirm, rename, or discard a recording after stopping
- Browse and delete saved recordings
- Play recordings with seek and 1x/2x/5x speed controls

## Dependencies

Run the bootstrap script once after cloning this repository:

```bash
./bootstrap.sh
```

It creates `.venv/`, installs the Python build tools, fetches dependencies from
`repos.json`, and keeps all third-party source under `dependencies/`.

System packages expected by the build:

- CMake and a C/C++ compiler
- SDL2 development files for `RECORDER_USE_SDL=ON`
- PulseAudio development files for `RECORDER_USE_PULSEAUDIO=ON`
- `python3-venv` so `bootstrap.sh` can create `.venv/`
- `aarch64-linux-gnu-gcc/g++` for cross-building the CardputerZero package

Project dependencies pulled from `repos.json`:

- `lvgl`
- `spdlog`
- `smooth_ui_toolkit`
- `miniaudio`

Image asset conversion runs during build and uses LVGL's Python converter. If
the converter dependencies are missing, install them in your Python environment:

```bash
./.venv/bin/python -m pip install pypng lz4 Pillow
```

Font conversion is not part of the normal build. If fonts are regenerated with
`src/assets/convert_fonts.py`, install `lv_font_conv` separately and keep it
available in `PATH`.

## Build

For Linux SDL testing:

```bash
cmake -S . -B build/sdl -DRECORDER_USE_SDL=ON
cmake --build build/sdl -j8
```

For CardputerZero framebuffer build:

```bash
cmake -S . -B build/cp0 -DRECORDER_USE_SDL=OFF -DRECORDER_USE_PULSEAUDIO=ON
cmake --build build/cp0 -j8
```

For cross build from x86 Linux with the GNU aarch64 toolchain:

```bash
cmake -S . -B build/cp0 \
  -DRECORDER_USE_SDL=OFF \
  -DRECORDER_USE_PULSEAUDIO=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-linux-gnu.cmake
cmake --build build/cp0 -j8
```

The output binary is `dist/M5CardputerZero-Recorder`.

## Usage

Run the SDL build:

```bash
./dist/M5CardputerZero-Recorder
```

Use a custom recordings directory:

```bash
./dist/M5CardputerZero-Recorder --recordings-dir ./recordings
```

Packaged CardputerZero launches use `$HOME/Recordings` by default.

Key controls:

- Recording page: `6` start/stop, `5` pause/resume, `4` waveform mode, `8` files, `Esc` exit
- Files page: `5`/`6` or Up/Down select, `7` or Enter play, `8` delete, `4` or Esc back
- Playback page: `5` back 10s, `6` play/pause, `7` forward 10s, `8` speed, `4` or Esc back

## Package

Build the CardputerZero Debian package:

```bash
./packaging/deb/package_deb.sh
```

The generated package is written to `dist/`:

```text
dist/m5cardputerzero-recorder_0.1.0_m5stack1_arm64.deb
```
