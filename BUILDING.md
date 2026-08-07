# Building REFLECT-84

JUCE 8.0.14 is fetched automatically by CMake on every platform — there is no submodule and no
local JUCE checkout to manage. The first configure downloads and builds JUCE's tooling, so expect
it to take a few minutes; subsequent configures are fast.

Re-run the configure step whenever `CMakeLists.txt` changes (new sources, new `juce_add_plugin`
arguments). A plain rebuild will not pick those up.

## Requirements

| Platform | Needs |
|---|---|
| macOS | Xcode 14+ (Command Line Tools alone are not enough — JUCE needs the full toolchain for AU), CMake 3.24+ |
| Windows | Visual Studio 2022 with the Desktop C++ workload, CMake 3.24+ |
| Linux | GCC 11+ or Clang 14+, CMake 3.24+, and JUCE's dependency list below |

Linux packages:

```sh
sudo apt install libasound2-dev libjack-jackd2-dev ladspa-sdk \
  libcurl4-openssl-dev libfreetype6-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev libwebkit2gtk-4.1-dev libglu1-mesa-dev mesa-common-dev
```

## Configure and build

macOS (AU + VST3 + Standalone):

```sh
cmake -B build -G Xcode
cmake --build build --config Release
```

Windows (VST3 + Standalone). No explicit `-G`: pinning a Visual Studio generator version breaks
whenever the installed VS moves on.

```bat
cmake -B build -A x64
cmake --build build --config Release
```

Linux (VST3 + Standalone). A single-config generator, so the build type has to be set at configure
time rather than only at build time:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Built plugins are copied into place automatically on macOS
(`~/Library/Audio/Plug-Ins/VST3` and `.../Components`) and land in JUCE's own per-OS defaults
elsewhere — `%COMMONPROGRAMFILES%\VST3\` on Windows, `~/.vst3/` on Linux.

The universal macOS build produces arm64 + x86_64. If a CI-published package ever turns out to be
single-architecture, the cause is almost certainly `CMAKE_OSX_ARCHITECTURES` having drifted below
`project()` in `CMakeLists.txt`, where it is a silent no-op.

## Tests

```sh
./build/Tests/Reflect84Tests_artefacts/Release/Reflect84Tests          # macOS / Linux
build\Tests\Reflect84Tests_artefacts\Release\Reflect84Tests.exe        # Windows
```

The suite is JUCE's own `UnitTest` framework, not Catch2, and returns a non-zero exit code if
anything fails. It covers `GrainSpec`'s formulas against the design document, the Program bank's
structure, the Program save/delete/cancel contract, and the reverb engine — RT60 tracking,
stability at maximum settings, the four algorithms being measurably distinct, switch continuity,
and a CPU budget check.

The CPU test logs per-algorithm timings; on an M-series Mac all four sit around 0.3–0.5% of the
real-time budget at 48 kHz / 256 samples.

## Validation

macOS:

```sh
auval -v aufx Rf84 Nfdy

/Applications/pluginval.app/Contents/MacOS/pluginval \
    --strictness-level 8 \
    --validate ~/Library/Audio/Plug-Ins/VST3/REFLECT-84.vst3
```

Windows:

```bat
pluginval.exe --strictness-level 8 --validate "%COMMONPROGRAMFILES%\VST3\REFLECT-84.vst3"
```

Linux:

```sh
./pluginval --strictness-level 8 --validate ~/.vst3/REFLECT-84.vst3
```

If Logic Pro does not pick up a freshly built AU: Audio Units Manager → "Reset & Rescan Selection",
or restart Logic. Note that a full `auval -a` instantiates every Audio Unit installed on the machine
and can take many minutes — validate by code (`auval -v aufx Rf84 Nfdy`) instead.

## DSP tuning

The four tank topologies are all real and complete, but their delay tables, diffusion coefficients
and early-reflection patterns are a structurally-reasoned first pass rather than one arrived at by
ear. The same is true of the twelve factory Programs: their values were authored deliberately, but
nothing has been listened to yet. Build, load, listen, adjust — and keep
`Tests/ReverbEngineTests.cpp`'s "measurably distinct" case passing while you do, because it is the
guard on the whole premise that these are four different reverbs rather than one with four names.
