# Coral Controller

Native Windows (ARM64) viewer/controller for the Coral Dev Board Micro. Requirements: [docs/REQUIREMENTS.md](docs/REQUIREMENTS.md).
Stage 1 (viewer) is implemented.

## Setup (once)

    winget configure -f setup\configuration.winget --accept-configuration-agreements
    git submodule update --init

System-wide: Git, Visual Studio 2022 Build Tools (MSVC ARM64, Windows SDK, CMake + Ninja). Everything else
(vcpkg, OpenCV, Dear ImGui, Boost, GoogleTest, build output) stays inside this folder. Undo with `scripts\cleanup.ps1`.

## Build and test

    .\scripts\build.ps1                    # ARM64 RelWithDebInfo, unit + integration tests
    .\scripts\build.ps1 -Config debug
    .\scripts\build.ps1 -HardwareTests     # also tests that need the board

App: `build\arm64-release\app\CoralController.exe` (snapshots go to `.\snapshots`).
Smoke run: `CoralController.exe --snapshot-after-frames 3` saves a raw + processed snapshot and exits.

## VS Code

Install the recommended extensions, pick the `ARM64 Debug` configure preset in CMake Tools, build, then use the
Run and Debug configurations "Coral Controller (app)", "Unit tests", "Integration tests", "Hardware tests".

## Layout

    src/common     Result, Clock
    src/device     USB VID/PID detection, Asio serial port, background reader, reconnecting connection manager
    src/protocol   base64 + FRAME text protocol parser
    src/imaging    JPEG decode, latest-frame buffer, snapshots
    src/filters    filter interface, parameters, registry, pipeline, built-in filters
    src/status     fps counter, status model
    src/ui         filter panel model (toolkit independent)
    src/viewer     reader-thread session (bytes -> frames)
    app/           Win32 + DX11 + Dear ImGui front end
    tests/         unit, integration (fakes), hardware

## Adding a filter

Write a `FilterBase` subclass with `static constexpr std::string_view kName`, its `FilterParameter`s and `apply()`,
then add one line `registry.add<MyFilter>();` in `src/filters/BuiltinFilters.cpp`. The panel UI is generated.