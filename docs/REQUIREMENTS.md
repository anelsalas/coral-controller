# Coral Controller - Requirements

Status: Approved v0.2 (2026-10-19). Stage 1 implementation in progress.

## 1. Purpose and scope

Coral Controller is a native Windows C++ desktop application for the Coral Dev Board Micro. It is delivered in three stages:

1. **Viewer** - auto-detect the board, show the live camera, apply live-tunable pluggable filters, take snapshots. Works with the current text protocol (`FRAME:<base64 JPEG>`).
2. **Binary streaming** - new board firmware streaming framed binary packets for higher frame rate; a shared protocol header used by board and PC; PC supports both protocols.
3. **Board controller** - manage a local library of board apps: list, install, restore default, optionally try out from RAM, with progress and automatic reconnect.

Out of scope: direct-USB flashing via hidapi (future option), the CDC-EEM network interface (no Windows driver), non-Windows hosts, containerized build or runtime.

## 2. Glossary

| Term | Meaning |
|---|---|
| Board | Coral Dev Board Micro (NXP i.MX RT1176, Edge TPU, 324x324 camera). |
| Board app | Firmware image running on the board. |
| Default app | Board app restored by "uninstall" / "restore default". |
| Text protocol | Current protocol: lines `FRAME:<base64 JPEG>\r\n` over USB serial. |
| Binary protocol | Stage 2 framed packet protocol (magic, version, type, sequence, length, CRC). |
| Filter | A single image operation (e.g. rotate, blur) with named, tunable parameters. |
| Pipeline | Ordered list of enabled filters applied to each frame. |
| Flash tools | NXP `elftosb`, `blhost`, `sdphost` bundled in the coralmicro repo. |
| Manifest | Per-app metadata file in `apps/<name>/`. |
| Hardware test | Test requiring a connected board; excluded from the default test run. |

## 3. Context and constraints

- Host: Microsoft Surface, Windows on ARM64, Windows PowerShell 5.1, user `anels`. All code is written and built on this machine.
- Board USB identity: VID `18D1`, PID `9308`. Appears as a USB serial port (currently COM8; the number must not be hard-coded).
- Current firmware: `camera_streaming_serial` (`C:\Users\anels\Source\coralmicro\camera_streaming_serial.cc`), roughly 1 fps (observed, not measured precisely), images arrive rotated 90 degrees.
- Official flasher: `C:\Users\anels\Source\coralmicro\scripts\flashtool.py`, which drives the bundled NXP tools. It serves as the reference for the flashing sequence; Python is not required at runtime.
- Libraries: OpenCV (image processing), Dear ImGui (UI, Win32 + DirectX 11 backend), GoogleTest (tests), Boost (only used components: Asio serial_port, JSON, CRC, Process), hidapi only if needed. All via vcpkg manifest mode inside the project folder, triplet `arm64-windows`.
- Serial settings for the current firmware: 115200 baud, 8N1.
- Style: Clean Code (small single-purpose classes and functions, intention-revealing names, named constants, explicit error handling, no swallowed errors, unit tests for every requirement).

## 4. Functional requirements

Format: ID - statement. *Rationale.* **AC:** acceptance criteria. **Test:** planned test (U = unit, I = integration with fakes, H = hardware).

### 4.1 Stage 1 - Viewer (VIEW)

**VIEW-001** - The app shall detect the board by enumerating serial ports and selecting the one whose USB VID/PID is 18D1/9308.
*Rationale:* COM number varies between machines and ports.
**AC:** With a fake port list containing the board on any COM name, that port is selected; with no match, the state is "not connected" and no port is opened.
**Test:** U `BoardDetector_SelectsPortMatchingVidPid`, U `BoardDetector_ReportsNotFoundWhenNoMatch`.

**VIEW-002** - The app shall connect automatically to the detected board and read the serial stream on a background thread without blocking the UI.
*Rationale:* UI must stay responsive.
**AC:** With a fake serial port that delays reads, the UI frame loop continues; received data reaches the frame queue.
**Test:** I `SerialReader_DeliversDataWithoutBlockingCaller`.

**VIEW-003** - The app shall parse text protocol lines `FRAME:<base64>\r\n` and extract the JPEG bytes; non-FRAME lines shall be passed to a board log view.
*Rationale:* Stage 1 must work with the current firmware.
**AC:** A valid line yields identical JPEG bytes; split across multiple reads it still yields one frame; a non-FRAME line is logged, not dropped.
**Test:** U `TextFrameParser_DecodesValidLine`, U `TextFrameParser_HandlesLineSplitAcrossReads`, U `TextFrameParser_RoutesNonFrameLinesToLog`.

**VIEW-004** - The app shall reject malformed frames (bad base64, undecodable JPEG, line exceeding a named maximum length) with a counted, logged error and continue streaming.
*Rationale:* No swallowed errors; one bad frame must not stop the stream.
**AC:** Each malformed input increments a specific error counter, emits a log entry, and the next valid frame displays.
**Test:** U `TextFrameParser_RejectsInvalidBase64`, U `TextFrameParser_RejectsOverlongLine`, U `JpegDecoder_ReportsCorruptImage`.

**VIEW-005** - The app shall decode JPEG frames and display the latest processed frame in an ImGui window.
**AC:** A known JPEG fixture decodes to the expected size (324x324); the display shows the most recent frame only (older queued frames are dropped).
**Test:** U `JpegDecoder_DecodesFixtureTo324x324`, U `LatestFrameBuffer_KeepsOnlyNewest`.

**VIEW-006** - The app shall apply a filter pipeline in which each filter implements a common interface (name, parameter descriptors, apply).
*Rationale:* Pluggable, independently testable filters.
**AC:** Pipeline applies enabled filters in order; disabled filters are skipped; a filter error is reported with the filter name and the unfiltered frame is displayed.
**Test:** U `FilterPipeline_AppliesEnabledFiltersInOrder`, U `FilterPipeline_SkipsDisabledFilters`, U `FilterPipeline_ReportsFailingFilterByName`.

**VIEW-007** - The app shall provide built-in filters: rotate (90/180/270), flip (horizontal/vertical), Gaussian blur, Canny edges, threshold, color map.
**AC:** Each filter produces the expected output on a small synthetic image.
**Test:** U `RotateFilter_Rotates90Clockwise`, U `FlipFilter_FlipsHorizontally`, U `BlurFilter_SmoothsImpulse`, U `EdgeFilter_DetectsStepEdge`, U `ThresholdFilter_BinarizesAtLevel`, U `ColorMapFilter_ProducesThreeChannels`.

**VIEW-008** - The ImGui panel shall list all registered filters, allow enable/disable and reorder, and render controls generated from each filter's parameter descriptors (int/float range, bool, enum); changes take effect on the next frame.
*Rationale:* Live tuning without per-filter UI code.
**AC:** Changing a parameter value through the panel model changes the filter's next output; parameter values are clamped to declared ranges.
**Test:** U `FilterParameters_ClampToDeclaredRange`, U `FilterPanelModel_UpdatesFilterParameter`.

**VIEW-009** - Adding a new filter shall require only one new class/file and one registration line; no changes to UI or pipeline code.
*Rationale:* "Easily programmable" filters.
**AC:** A test-only filter registered with one line appears in the registry and the panel model.
**Test:** U `FilterRegistry_ExposesNewlyRegisteredFilter`.

**VIEW-010** - The app shall apply a rotation correction by default so that frames from the current firmware display upright; the default shall be user-changeable.
*Rationale:* Current images arrive rotated 90 degrees.
**AC:** Default pipeline output is upright on hardware; the rotate filter is present and preconfigured, enabled only if the hardware needs it (D-13).
**Test:** U `DefaultPipeline_IncludesRotationCorrection`; H `Hardware_ViewerShowsUprightImage` (manual check).

**VIEW-011** - The app shall save a snapshot of the currently displayed (processed) frame, and optionally the raw frame, to a PNG file with a timestamped name in a configurable folder.
**AC:** File exists, decodes, matches displayed frame dimensions; write failure is reported to the user.
**Test:** U `SnapshotWriter_WritesTimestampedPng`, U `SnapshotWriter_ReportsWriteFailure`.

**VIEW-012** - The app shall detect disconnect (read error or unplug), close the port, and reconnect automatically when the board reappears, without restarting the app.
**AC:** With a fake port that fails then reappears, state goes Connected -> Disconnected -> Connected and streaming resumes.
**Test:** I `ConnectionManager_ReconnectsAfterPortLoss`; H `Hardware_ReconnectsAfterReplug`.

**VIEW-013** - The app shall display status: connection state, port name, received fps, decode error counts, active protocol, and board app name/version when known ("unknown" for the text protocol).
**AC:** Status model reflects fake connection events; fps computed over a sliding window from frame timestamps.
**Test:** U `FpsCounter_ComputesRateOverWindow`, U `StatusModel_ReflectsConnectionEvents`.

### 4.2 Stage 2 - Binary protocol and firmware (PROTO)

**PROTO-001** - A single shared C/C++ protocol header shall define magic, protocol version, packet types, header layout, maximum payload size, and CRC algorithm, and shall compile both in the board firmware and in the PC app.
*Rationale:* One source of truth prevents drift.
**AC:** Header compiles with the Arm GNU toolchain (no exceptions, no RTTI) and with MSVC; static asserts check header size and field offsets.
**Test:** U `ProtocolHeader_LayoutMatchesSpec` (static asserts + runtime check); build check BUILD-007.

**PROTO-002** - Each packet shall consist of: magic, version, type, sequence number, payload length, payload, CRC, with all multi-byte fields in a defined byte order.
**AC:** Encoding a known packet produces a golden byte sequence; decoding it returns identical fields.
**Test:** U `PacketCodec_EncodesGoldenPacket`, U `PacketCodec_RoundTripsAllTypes`.

**PROTO-003** - The PC decoder shall verify CRC and length, reject packets with bad CRC, unknown version, or length above the maximum, and count each rejection by reason.
**AC:** Each corrupted fixture is rejected with the correct reason; no payload is delivered.
**Test:** U `PacketDecoder_RejectsBadCrc`, U `PacketDecoder_RejectsOversizeLength`, U `PacketDecoder_RejectsUnknownVersion`.

**PROTO-004** - The PC decoder shall resynchronize on the next magic after garbage or a rejected packet.
**AC:** Stream = garbage + valid packet + corrupt packet + valid packet yields exactly the two valid packets.
**Test:** U `PacketDecoder_ResyncsAfterGarbage`.

**PROTO-005** - The PC shall detect sequence gaps and report dropped-packet counts.
**AC:** Sequences 1,2,5 report 2 dropped; wrap-around is not counted as a gap.
**Test:** U `SequenceTracker_CountsGaps`, U `SequenceTracker_HandlesWrapAround`.

**PROTO-006** - The board app shall send a HELLO packet carrying app name and app version at startup and on PC request (or periodically), and the PC shall show them in status.
**AC:** Decoding a HELLO fixture updates the status model's app name/version.
**Test:** U `HelloPacket_UpdatesBoardIdentity`; H `Hardware_BoardAnnouncesIdentity`.

**PROTO-007** - A new board firmware app shall capture camera frames and stream them as binary FRAME packets (JPEG payload) over USB serial, without base64.
*Rationale:* Base64 adds ~33% overhead and text parsing cost.
**AC:** Viewer displays frames from the new firmware; measured fps recorded and compared to text protocol (target in NFR PERF-002).
**Test:** H `Hardware_BinaryFirmwareStreamsFrames`.

**PROTO-008** - Board-side code shall be embedded-friendly: no exceptions, no dynamic allocation after initialization (statically bounded buffers), FreeRTOS tasks, all error paths returning explicit status codes.
**AC:** Firmware builds with `-fno-exceptions`; code review checklist confirms no heap use in the streaming loop; encoder unit tests run on host.
**Test:** U `FirmwarePacketEncoder_FailsOnOversizePayload` (host-built firmware module).

**PROTO-009** - The PC app shall auto-detect the protocol in use (text vs binary) from the incoming byte stream and switch decoders without user action.
**AC:** A stream starting with `FRAME:` selects the text decoder; one starting with magic selects binary; a firmware change after reconnect is handled.
**Test:** U `ProtocolDetector_SelectsTextForFrameLines`, U `ProtocolDetector_SelectsBinaryForMagic`.

**PROTO-010** - The PC shall report incompatible protocol versions to the user instead of attempting to decode.
**AC:** HELLO with a higher major version produces a visible "incompatible firmware" status.
**Test:** U `VersionCheck_FlagsIncompatibleMajorVersion`.

### 4.3 Stage 3 - Board controller (FLASH)

**FLASH-001** - The app shall read an app library folder where each board app is `apps/<name>/` containing a firmware image and a manifest.
**AC:** Fixture library with valid, missing-firmware, and missing-manifest entries lists only the valid one and reports the others with reasons.
**Test:** U `AppLibrary_ListsValidApps`, U `AppLibrary_ReportsInvalidEntries`.

**FLASH-002** - The manifest shall declare at least: name, version, description, firmware file name, protocol (text/binary), and whether it is the default app; the schema shall be documented.
**AC:** Valid manifest parses; missing required fields fail with field name.
**Test:** U `ManifestParser_ParsesValidManifest`, U `ManifestParser_NamesMissingField`.

**FLASH-003** - The UI shall list library apps with name, version, description, and mark the app currently running on the board (from HELLO, when available).
**AC:** Panel model contains all apps; running app marked when identity matches.
**Test:** U `AppListModel_MarksRunningApp`.

**FLASH-004** - The app shall install (flash) a selected app by running the bundled NXP flash tools in the sequence used by `flashtool.py`, from C++, with no Python at runtime.
*Rationale:* Self-contained tool; flashtool.py is the reference sequence.
**AC:** With a fake process runner, the expected commands and arguments are issued in order; non-zero exit stops the sequence.
**Test:** I `Flasher_IssuesCommandSequenceInOrder`, I `Flasher_StopsOnToolFailure`; H `Hardware_InstallsLibraryApp`.

**FLASH-005** - "Uninstall" shall be implemented as restoring (flashing) the default app; exactly one app shall be marked default.
**AC:** Restore-default flashes the manifest-marked default; zero or multiple defaults are reported as library errors.
**Test:** U `AppLibrary_RequiresExactlyOneDefault`, I `Flasher_RestoreDefaultFlashesDefaultApp`.

**FLASH-006** - *(To be verified)* If the Coral flash tooling supports loading an image into RAM without writing flash, the app shall offer "try out" (temporary run until reset). If not supported, this requirement is dropped.
**AC:** Verification result recorded in Decisions; if supported, try-out leaves the previously installed app running after power cycle.
**Test:** H `Hardware_TryOutRevertsAfterReset` (only if supported).

**FLASH-007** - The app shall show flashing progress (current step, percent where the tool reports it) and the final result, and allow no other board operation during flashing.
**AC:** Fake tool output with progress lines maps to progress model values; UI actions disabled while busy.
**Test:** U `ToolOutputParser_ExtractsProgress`, U `ControllerState_BlocksActionsWhileFlashing`.

**FLASH-008** - Before flashing, the app shall stop streaming and release the COM port; after flashing it shall wait for the board to re-enumerate and reconnect.
**AC:** Fake sequence shows port closed before first tool call and reopened after success; timeout produces a clear error.
**Test:** I `FlashSession_ReleasesPortBeforeFlashing`, I `FlashSession_ReconnectsAfterFlashing`, I `FlashSession_ReportsReconnectTimeout`.

**FLASH-009** - Every flash tool failure (missing executable, non-zero exit, timeout) shall be reported with the tool name, exit code, and captured output; the app shall never report success on failure.
**AC:** Each failure type produces a distinct error result containing captured output.
**Test:** U `ProcessRunner_ReportsMissingExecutable`, I `Flasher_ReportsTimeoutWithOutput`.

**FLASH-010** - Paths to the flash tools shall be configurable, defaulting to the coralmicro repo's bundled location, and validated at startup.
**AC:** Missing tool path shows a configuration error; flashing is disabled until fixed.
**Test:** U `FlashToolConfig_ValidatesPaths`.

**FLASH-011** - After install, the app shall verify the board runs the expected app (via HELLO when the app supports it).
**AC:** Mismatched HELLO after flashing is reported as a warning.
**Test:** I `FlashSession_VerifiesInstalledIdentity`.

### 4.4 Build and environment (BUILD)

**BUILD-001** - A winget configuration file (DSC YAML) shall declare the system tools: Visual Studio Build Tools (C++ workload, ARM64), CMake, Git, Arm GNU toolchain (stage 2).
**AC:** `winget configure` on a clean machine installs the tools; re-running is a no-op.
**Test:** Manual `Setup_WingetConfigureIsIdempotent`.

**BUILD-002** - Libraries shall come from vcpkg in manifest mode (`vcpkg.json` with builtin-baseline and version pins), installed inside the project folder.
**AC:** No library files are installed outside the project folder; the same baseline yields the same versions.
**Test:** Manual/CI `Build_VcpkgInstallsIntoProjectTree`.

**BUILD-003** - `CMakePresets.json` shall define configure/build/test presets (at least Debug and Release for the host app).
**AC:** `cmake --preset` and `ctest --preset` succeed on the Surface.
**Test:** `Build_PresetsConfigureAndTest`.

**BUILD-004** - A single build script shall configure, build, and run unit tests in one command.
**AC:** One command from a fresh clone (after BUILD-001) produces the executable and a test report.
**Test:** `Build_SingleScriptBuildsAndTests`.

**BUILD-005** - Tests shall be labeled `unit`, `integration`, and `hardware`; the default test run shall exclude `hardware`.
**AC:** Default run passes with no board connected; hardware tests run only on explicit selection.
**Test:** `Build_DefaultTestRunExcludesHardware`.

**BUILD-006** - A documented cleanup path shall remove everything the project installed (project folder, vcpkg caches, winget-installed tools optionally).
**AC:** Following the document leaves no project artifacts outside documented locations.
**Test:** Manual `Setup_CleanupRemovesArtifacts`.

**BUILD-007** - Board firmware for stage 2 shall build on the Surface with the Arm GNU toolchain against the coralmicro SDK.
**AC:** Firmware image produced by a documented command/preset.
**Test:** `Build_FirmwareBuildsWithArmGnu`.

## 5. Non-functional requirements

| ID | Requirement | Verification |
|---|---|---|
| CC-001 | Classes/functions shall be small and single-purpose; names intention-revealing; no magic numbers (named constants). | Code review checklist. |
| CC-002 | Errors shall be returned or thrown explicitly and handled or surfaced; no empty catch blocks or ignored status codes. | Review + compiler warnings as errors (`/W4 /WX`). |
| CC-003 | Every functional requirement shall have at least one automated test (hardware tests may be manual where noted). | Traceability table (sec. 8). |
| CC-004 | Hardware access (serial port, process execution, file system, clock) shall be behind interfaces with fakes for tests. | Review; integration tests use fakes. |
| PERF-001 | Target: UI remains responsive (>= 30 UI fps) while streaming and filtering at 324x324. Target, to be measured. | Measure in app status. |
| PERF-002 | Target: binary protocol achieves materially higher fps than text protocol (goal >= 5 fps; actual to be measured). | H `Hardware.BinaryFirmwareStreamsFrames` (asserts > 1 fps); measured with `CoralController.exe --measure-fps 30` (D-17). |
| PERF-003 | Target: frame display latency kept to the latest frame (no unbounded queue). | U `LatestFrameBuffer_KeepsOnlyNewest`. |
| REL-001 | No crash on malformed input, unplug, or tool failure; all such events logged. | Fuzz-style parser tests, VIEW-012, FLASH-009. |
| REL-002 | Bounded memory: input buffers and queues have named maximum sizes. | Unit tests on limits. |
| PORT-001 | Host app targets Windows 11 ARM64 natively; x64 build under emulation is an acceptable fallback (see OQ-1). | Build presets. |
| PORT-002 | Protocol and filter logic shall not depend on Windows APIs (platform code isolated in one module). | Review. |
| FOOT-001 | System-wide footprint limited to the tools in BUILD-001; everything else inside the project folder. | BUILD-002, BUILD-006. |

## 6. Architecture overview

Modules (host app):

- **device** - port enumeration by VID/PID, serial port wrapper, connection manager (connect/reconnect state machine).
- **protocol** - text frame parser, binary packet codec, protocol detector, sequence tracker; uses the shared protocol header.
- **imaging** - JPEG decode, latest-frame buffer, filter interface, filter registry, pipeline, built-in filters (OpenCV).
- **ui** - Dear ImGui windows: video view, filter panel (generated from parameter descriptors), status, board log, app library.
- **library** - app library scanner, manifest parser.
- **flashing** - process runner, flash tool config, flasher (command sequence), flash session (port release/reconnect/verify), tool output parser.
- **app** - composition root wiring modules; owns threads.
- **shared/protocol** - header shared with firmware.
- **firmware/** - stage 2 board app (FreeRTOS, coralmicro SDK).

Data flow: serial reader thread -> protocol decoder -> JPEG decode -> latest-frame buffer -> UI thread runs pipeline and renders.

## 7. Decisions and open questions

### Decisions

- **D-1** Native Windows C++ with OpenCV + Dear ImGui (Win32 + DirectX 11 backend).
- **D-2** Containers rejected: runtime needs GUI and COM port access; build via Docker is heavy and weak on Windows ARM. Use winget DSC + vcpkg manifest instead.
- **D-3** Flashing drives NXP bundled executables from C++; no Python at runtime. Direct-USB (hidapi) flashing is a future option, out of scope.
- **D-4** Uninstall = flash the default app.
- **D-5** Text protocol remains supported permanently alongside binary.
- **D-6** CDC-EEM network interface not used (no Windows driver); all traffic over USB serial.
- **D-7** (resolves OQ-1) Native ARM64 build, vcpkg triplet `arm64-windows`. Fall back to x64 only for a specific library that truly fails to build or run on ARM64, and record that here.
- **D-8** (resolves OQ-5) GoogleTest is the test framework; tests registered with CTest and labeled `unit`, `integration`, `hardware`.
- **D-9** (resolves OQ-7) Manifests are JSON, parsed with Boost.JSON.
- **D-10** (resolves OQ-6, partly) Binary protocol CRC is CRC-32 (Boost.CRC on the PC; the same constexpr table implementation from shared\protocol is used by firmware and PC; Boost.CRC is the test oracle). Maximum payload size: see D-14.
- **D-11** Boost via vcpkg, only the used components: Boost.Asio `serial_port` (serial link, stage 1), Boost.JSON (manifests, stage 3), Boost.CRC (stage 2), Boost.Process (flash tools, stage 3).
- **D-13** (resolves OQ-4) Hardware check 2026-10-19: frames from camera_streaming_serial arrive upright in this setup. The default pipeline keeps a preconfigured Rotate stage (90 CW) disabled; VIEW-010 AC is "default output is upright".
- **D-14** (resolves OQ-6b) CRLP v1 kMaxPayloadBytes = 128 KiB. A frame payload is an 8-byte FrameInfo plus the JPEG; the firmware JPEG buffer is sized to fit.
- **D-15** (resolves OQ-2) flashtool.py supports `--ram` (run from RAM, reverts on reset); exposed as `flash-firmware.ps1 -Ram`.
- **D-16** Firmware: coralmicro SDK as a git submodule (third_party\coralmicro @ c9f665b), built out-of-tree with VS CMake/Ninja and the SDK-pinned Arm GCC 9-2020-q2. The SDK is unmodified. camera_stream_binary disables the 115200 UART console mirror (DbgConsole_Deinit) and sends only over USB CDC in 512-byte chunks.
- **D-17** Measured 2026-10-19 (Surface, 30 s window): text camera_streaming_serial 0.70 fps; binary camera_stream_binary 18.56 fps (0 dropped/rejected packets); hardware test window 21.3 fps.
- **D-18** (open issue) USB CDC corruption on the board: coralmicro `CdcAcm::Transmit` waits 200 ms for completion; if the host closes the port mid-transfer, the late completion leaves the binary tx semaphore one give ahead. From then on, Transmit returns before its transfer finishes and the next chunk overwrites the single tx buffer. Measured 2026-10-19 before the workaround: 1 of 6 open/close cycles put the board into the corrupt state (100% CRC failures until reset). The SDK exposes no public way to wait for or cancel the pending transfer, or to clear the semaphore, and ConsoleM7 reports no transmit result to the app. App-only mitigation: PacedChunkSink (minimum gap between chunk starts). Measured with a 300 us gap: 40/40 two-second cycles clean (~20.5 fps) and 2 x 5.5 min idle then open clean (20.7 / 19.4 fps), but one 30 s run had 158 rejected packets (14.3 fps). With 1000 us: 4-6 rejects per 30 s, 11.7 fps. With 500 us: 10/10 cycles bad and then the board's USB stopped responding. Conclusion: mitigated, not fixed; a full fix needs an SDK change (e.g. drain the semaphore before Send, or cancel the transfer on timeout). Shipped gap: 300 us.
- **D-19** The UI thread never touches the device: ConnectionManager runs find/open/read/close on one worker thread; `shutdown()` waits at most 2 s and abandons a stuck worker (no further callbacks); the app then exits with code 4 without running destructors. Found when the viewer hung (white window, Not Responding) in a serial close while the board's USB was wedged.- **D-12** CMake and Ninja come from the Visual Studio Build Tools "C++ CMake tools" component instead of separate system-wide installs (smaller footprint).

### Open questions

- **OQ-3** Do the bundled NXP tools (elftosb/blhost/sdphost) have Windows ARM64 binaries, or do they run under x64 emulation?
- **OQ-8** Does the board's USB VID/PID change in bootloader/flashing mode, and how does reconnect detection handle it?
- **OQ-9** Can the board receive PC->board commands (e.g. HELLO request) on the same serial channel in stage 2?

Resolved: OQ-4 (D-13), OQ-1 (D-7), OQ-5 (D-8), OQ-6 CRC variant (D-10), OQ-6b (D-14), OQ-2 (D-15), OQ-7 (D-9).

## 8. Traceability

| Requirement | Planned test(s) |
|---|---|
| VIEW-001 | BoardDetector_SelectsPortMatchingVidPid, BoardDetector_ReportsNotFoundWhenNoMatch |
| VIEW-002 | SerialReader_DeliversDataWithoutBlockingCaller |
| VIEW-003 | TextFrameParser_DecodesValidLine, TextFrameParser_HandlesLineSplitAcrossReads, TextFrameParser_RoutesNonFrameLinesToLog |
| VIEW-004 | TextFrameParser_RejectsInvalidBase64, TextFrameParser_RejectsOverlongLine, JpegDecoder_ReportsCorruptImage |
| VIEW-005 | JpegDecoder_DecodesFixtureTo324x324, LatestFrameBuffer_KeepsOnlyNewest |
| VIEW-006 | FilterPipeline_AppliesEnabledFiltersInOrder, FilterPipeline_SkipsDisabledFilters, FilterPipeline_ReportsFailingFilterByName |
| VIEW-007 | RotateFilter_Rotates90Clockwise, FlipFilter_FlipsHorizontally, BlurFilter_SmoothsImpulse, EdgeFilter_DetectsStepEdge, ThresholdFilter_BinarizesAtLevel, ColorMapFilter_ProducesThreeChannels |
| VIEW-008 | FilterParameters_ClampToDeclaredRange, FilterPanelModel_UpdatesFilterParameter |
| VIEW-009 | FilterRegistry_ExposesNewlyRegisteredFilter |
| VIEW-010 | DefaultPipeline_IncludesRotationCorrection, Hardware_ViewerShowsUprightImage (H) |
| VIEW-011 | SnapshotWriter_WritesTimestampedPng, SnapshotWriter_ReportsWriteFailure |
| VIEW-012 | ConnectionManager_ReconnectsAfterPortLoss, Hardware_ReconnectsAfterReplug (H) |
| VIEW-013 | FpsCounter_ComputesRateOverWindow, StatusModel_ReflectsConnectionEvents |
| PROTO-001 | ProtocolHeader.LayoutMatchesSpec, ProtocolHeader.RejectsMissingMagic; firmware build `scripts\build-firmware.ps1` (compiles shared\protocol with Arm GCC) |
| PROTO-002 | PacketCodec.EncodesGoldenPacket, PacketCodec.RoundTripsAllTypes, PacketCodec.DecodesByteByByte, Crc32.MatchesStandardCheckValue, Crc32.MatchesBoostCrc32AndIsIncremental |
| PROTO-003 | PacketDecoder.RejectsBadCrc, PacketDecoder.RejectsOversizeLength, PacketDecoder.RejectsUnknownVersion |
| PROTO-004 | PacketDecoder.ResyncsAfterGarbage |
| PROTO-005 | SequenceTracker.CountsGaps, SequenceTracker.HandlesWrapAround, SequenceTracker.TreatsBackwardJumpAsRestart, BinaryViewerSessionTest.ReportsDroppedPackets |
| PROTO-006 | BinaryViewerSessionTest.HelloPacketUpdatesBoardIdentity, Hardware.BoardAnnouncesIdentity (H) |
| PROTO-007 | BinaryViewerSessionTest.FramePacketReachesDisplay, PacedChunkSink.* (6 tests, D-18), Hardware.BinaryFirmwareStreamsFrames (H) |
| PROTO-008 | FirmwarePacketEncoder.FailsOnOversizePayload, FirmwarePacketEncoder.ReportsSinkFailureAndConsumesSequence, FirmwarePacketEncoder.SplitPayloadEqualsContiguousPayload |
| PROTO-009 | ProtocolDetector.SelectsTextForFrameLines, ProtocolDetector.SelectsBinaryForMagic, ProtocolDetector.WaitsUntilDecisionIsPossible, ProtocolDetector.FallsBackToTextAfterProbeLimit, BinaryViewerSessionTest.TextProtocolStillWorks, BinaryViewerSessionTest.ReconnectRedetectsProtocol |
| PROTO-010 | BinaryViewerSessionTest.VersionCheckFlagsIncompatibleMajorVersion |
| FLASH-001 | AppLibrary_ListsValidApps, AppLibrary_ReportsInvalidEntries |
| FLASH-002 | ManifestParser_ParsesValidManifest, ManifestParser_NamesMissingField |
| FLASH-003 | AppListModel_MarksRunningApp |
| FLASH-004 | Flasher_IssuesCommandSequenceInOrder, Flasher_StopsOnToolFailure, Hardware_InstallsLibraryApp (H) |
| FLASH-005 | AppLibrary_RequiresExactlyOneDefault, Flasher_RestoreDefaultFlashesDefaultApp |
| FLASH-006 | Hardware_TryOutRevertsAfterReset (H, if supported) |
| FLASH-007 | ToolOutputParser_ExtractsProgress, ControllerState_BlocksActionsWhileFlashing |
| FLASH-008 | FlashSession_ReleasesPortBeforeFlashing, FlashSession_ReconnectsAfterFlashing, FlashSession_ReportsReconnectTimeout |
| FLASH-009 | ProcessRunner_ReportsMissingExecutable, Flasher_ReportsTimeoutWithOutput |
| FLASH-010 | FlashToolConfig_ValidatesPaths |
| FLASH-011 | FlashSession_VerifiesInstalledIdentity |
| BUILD-001 | Setup_WingetConfigureIsIdempotent (manual) |
| BUILD-002 | Build_VcpkgInstallsIntoProjectTree |
| BUILD-003 | Build_PresetsConfigureAndTest |
| BUILD-004 | Build_SingleScriptBuildsAndTests |
| BUILD-005 | Build_DefaultTestRunExcludesHardware |
| BUILD-006 | Setup_CleanupRemovesArtifacts (manual) |
| BUILD-007 | Build_FirmwareBuildsWithArmGnu |