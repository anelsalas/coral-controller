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
## Firmware (Stage 2: camera_stream_binary)

Board app in `firmware\camera_stream_binary`, built natively on Windows against the
`third_party\coralmicro` submodule (`git submodule update --init --recursive`; `core.longpaths=true`).
It shares `shared\protocol` (CRLP v1) with the PC app and streams JPEG frames over USB CDC
(UART mirror disabled), announcing `camera_stream_binary 1.0.0` at startup and every 30 frames.

The Arm GNU toolchain (gcc-arm-none-eabi 9-2020-q2, pinned by the SDK) is downloaded
by the SDK into `third_party\coralmicro\third_party\toolchain-win` on the first configure.

VS Code: *Terminal > Run Task*:
| Task | Script |
|---|---|
| Firmware: build camera_stream_binary | `scripts\build-firmware.ps1` -> `build\firmware` |
| Firmware: flash camera_stream_binary | `scripts\flash-firmware.ps1` |
| Firmware: run camera_stream_binary from RAM | `scripts\flash-firmware.ps1 -Ram` (reverts on reset) |
| Firmware: restore camera_streaming_serial (text) | `scripts\flash-firmware.ps1 -RestoreTextApp` |
| Tests: hardware (board connected) | `scripts\test-hardware.ps1` |

Flashing uses `C:\Users\anels\Source\coralmicro\scripts\flashtool.py` and its venv
(libusb from the venv's `libusb-package`). Close Coral Controller before flashing.
`CoralController.exe --measure-fps 30` prints the measured fps and the detected protocol.

Known issue (open, see REQUIREMENTS D-18): after the PC closes COM8 while a USB transfer is in flight, the SDK's
`CdcAcm::Transmit` can be left one semaphore "give" ahead, so later 512-byte chunks overwrite the transfer still
on the wire (viewer: "packet CRC mismatch", rising rejected/dropped counts). camera_stream_binary paces chunks
(PacedChunkSink) to reduce this, but it does not fully fix it, and once the board's USB stopped responding
completely. Recovery: unplug and replug the board. A full fix needs a change in the SDK (not done; pending your decision).
If the board's USB hangs, Coral Controller stays responsive and exits after 2 s (exit code 4,
"Board did not release the serial port"); Windows may keep the process entry until the board is replugged.