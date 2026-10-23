# AL STUDIO

**AL STUDIO** is an original Windows-native music-making project with a dark cyber-blue interface.

## First playable MVP
- 16-step beat sequencer with Kick, Snare, Closed Hat and Bass lanes
- Starter beat that you can edit by clicking the step buttons
- BPM field (60–180 BPM)
- Play and stop preview
- Export a four-bar stereo WAV file
- Windows EXE built by GitHub Actions

This is an early foundation, not yet a complete FL Studio replacement. Piano roll, playlist/arrangement, mixer effects, audio recording, MIDI support and VST3 hosting are future milestones.

## Download the Windows app
1. Open the repository's **Actions** tab.
2. Select **Build AL STUDIO for Windows**.
3. Open the latest successful run on the `main` branch.
4. Under **Artifacts**, download `AL-STUDIO-Windows`.
5. Extract the ZIP and run `AL-STUDIO.exe`.

The EXE is only available after the workflow finishes successfully.

## Build locally (optional)
Install Visual Studio 2022 Build Tools with the **Desktop development with C++** workload and CMake. Then, from a Developer PowerShell:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The executable is written under `build/Release/AL-STUDIO.exe` for a typical multi-configuration generator.

## Project layout
- `src/main.cpp` — Windows UI, step pattern, simple synthesized sounds and WAV export
- `CMakeLists.txt` — native Windows build
- `.github/workflows/windows-build.yml` — automated Windows build and artifact upload

## Audio notes
This MVP uses Windows' built-in playback facilities and a simple procedural drum/bass generator. It is not yet a low-latency, multi-track professional audio engine.
