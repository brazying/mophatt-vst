# Mo'Phatt VST

A Windows x64 VST3 sampler project for E-MU Emulator X / Proteus X bank files.

## Current development status

- JUCE VST3 + Standalone project.
- EXB bank browser and TOC2 preset/sample-link parsing.
- EBL v1/v2 parser for 16-bit PCM and 32-bit floating-point sample data.
- Mono and split-channel stereo EBL decoding.
- MIDI-playable EBL sample preview with basic looping and release.
- Multiple EXB banks can be imported; paths are saved in the DAW project state.
- GitHub Actions builds the Windows x64 VST3 and publishes it as a workflow artifact.

**Important:** this milestone can preview a selected EBL file over MIDI. It does not yet reconstruct full EXB presets, voice/key/velocity zones, or all Emulator X synthesis parameters. That is the next major stage.

## Bank layout

A standard Emulator X bank consists of an `.exb` file and a sibling `SamplePool` directory containing `.ebl` files. Common EBL names follow `<bank name>SL<sample index>.ebl`. Keep the original folder structure intact when importing a bank.

The plugin also has a direct EBL preview loader so sample decoding can be tested before full preset-zone support is finished.

## Build locally

Requirements:
- Windows 10/11 x64
- Visual Studio 2022 with the C++ desktop workload
- CMake 3.22+
- Internet connection on first configure (JUCE is fetched by CMake)

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

The VST3 target is built under `build/MoPhattVST_artefacts/Release/VST3/`.

## Roadmap

1. Complete EXB preset/voice/zone parsing.
2. Resolve each zone's sample index to its EBL file in SamplePool.
3. Play complete presets with key and velocity ranges.
4. Implement per-zone tuning, gain, pan, envelopes, filters and loops.
5. Add a persistent multi-bank library browser and preset search.
6. Expand compatibility across user-created EXB banks and single-file sample pools.

The project does not bundle E-MU factory sample content. Users select their own bank files and keep their SamplePool folders on their own computer.
