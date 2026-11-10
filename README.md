# Mo'Phatt VST

A Windows x64 VST3 sampler project focused on E-MU Emulator X / Proteus X bank support.

## Current milestone

- JUCE-based VST3 + Standalone project scaffold.
- EXB bank selection from the plugin UI.
- Initial EXB container validation and preset-name extraction.
- Multiple banks can be added to the browser.
- Audio playback is not implemented yet; that requires parsing the EXB preset/zone data and the matching EBL files in each bank's `SamplePool`.

## Bank layout

An Emulator X bank normally consists of an `.exb` file and its associated `SamplePool` directory containing `.ebl` sample files. The EXB file describes presets and links; the PCM audio is stored in EBL files. Keep the original folder layout together when importing a bank.

The plugin is intended to load multiple independent banks. Each imported bank is tracked by its EXB path, and the matching SamplePool is resolved relative to that path.

## Build

Requirements:
- Windows 10/11 x64
- Visual Studio 2022 with C++ desktop workload
- CMake 3.22+
- Internet connection on first configure (JUCE is fetched by CMake)

Configure and build:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

The VST3 target is built under `build/MoPhattVST_artefacts/Release/VST3/`.

## Format support roadmap

1. EXB container and preset metadata.
2. EBL sample decoding (16-bit and 32-bit, mono/stereo).
3. Preset zones, key/velocity ranges, tuning, gain, pan and looping.
4. Envelopes and supported filter settings.
5. User bank import and persistent bank library.
6. Expanded compatibility with user-created EXB banks.

This project does not include E-MU factory sample content. Users select their own legally obtained banks and keep their SamplePool folders on their own computer.
