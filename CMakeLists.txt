cmake_minimum_required(VERSION 3.24)
# LANGUAGES needs C as well as CXX: JUCE/VST3-SDK/other fetched third-party deps bundle some .c
# sources, and Xcode/Visual Studio tolerate that without an explicit enable_language(C), but
# Linux's single-config Makefiles/Ninja generator does not - it fails at CMake's generate step
# with "CMAKE_C_COMPILE_OBJECT not set" if C was never enabled at the top level.
# These MUST come before project(): project() creates these cache entries itself, after which a
# plain set(... CACHE ...) is a silent no-op. Sitting below project() they never take effect -
# builds inherit the host architecture and the default SDK deployment target instead, which only
# surfaces when a CI-published .pkg turns out to contain an arm64-only binary.
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal: Apple Silicon + Intel")
endif()

project(Reflect84 VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------------------------
# neon-foundry-core: shared BEHAVIOUR across the suite, pinned by tag exactly like JUCE above.
#
# **This is not a dependency on a sibling casting.** Depending on one of those is still forbidden —
# it couples two things that must ship on their own schedules. This is a versioned library that no
# casting owns, so a change to it forces nothing on anyone until they move their own pin, which is
# the same relationship every casting already has with JUCE. The root CLAUDE.md draws that
# distinction under "Structure".
#
# **Declared AFTER JUCE, and that ordering is load-bearing.** Core links juce::juce_core and does
# not fetch its own when consumed; it fails with an explicit message if JUCE is not already
# available. Two JUCE trees in one build link two juce_core builds into one binary and surface as
# duplicate symbols a long way from the cause.
#
FetchContent_Declare(NeonFoundryCore
    GIT_REPOSITORY https://github.com/trudslev/neon-foundry-core.git
    GIT_TAG v0.1.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(NeonFoundryCore)

# REFLECT-84's panel is entirely vector/code-drawn - unlike Gatecrasher and CHORUS-60 there is no
# background plate, no knob filmstrip, no baked nameplate. The only runtime assets are the two
# typefaces design/README.md specifies: Jost 500 for the engraved wordmark and IBM Plex Mono for
# every other glyph on the panel. Both are OFL (licences alongside them in design/fonts/). The
# screenshots in design/screenshots/ are pixel-matching acceptance targets, not runtime assets, so
# they are deliberately not embedded here.
# NOTE on JUCE's name mangling: it STRIPS non-alphanumeric characters rather than converting them
# to underscores, so Jost-500-Medium.ttf becomes BinaryData::Jost500Medium_ttf - not
# Jost_500_Medium_ttf. Check the generated BinaryData.h before assuming a symbol name.
juce_add_binary_data(Reflect84BinaryData
    SOURCES
        design/fonts/jost/Jost-500-Medium.ttf
        design/fonts/ibm-plex-mono/IBMPlexMono-Regular.ttf
        design/fonts/ibm-plex-mono/IBMPlexMono-Medium.ttf)

# AU is Apple-only; Windows/Linux only ever build VST3 + Standalone. VST3_COPY_DIR/AU_COPY_DIR are
# only set on Apple - leaving them unset elsewhere lets JUCE apply its own correct per-OS default
# rather than a macOS path built on a non-guaranteed $ENV{HOME}.
if(APPLE)
    set(REFLECT84_FORMATS AU VST3 Standalone)
    set(REFLECT84_EXTRA_ARGS
        VST3_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
        AU_COPY_DIR   "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
else()
    set(REFLECT84_FORMATS VST3 Standalone)
    set(REFLECT84_EXTRA_ARGS "")
endif()

# Single source of truth for the two identity strings that appear BOTH in the bundle metadata and
# in ProgramManager's user-Programs directory path. ProgramManager.cpp can't read JUCE's
# JucePlugin_* macros - they only exist in the plugin target's generated header, and that file is
# also compiled into the Tests console app. CHORUS-60's CMakeLists records what happens when this
# is hand-synced instead: COMPANY_NAME became "Neon Foundry" while the copy still said "Tanis",
# quietly pointing saved Programs at the old directory. Handing both targets the same values from
# here makes drift a build error rather than silent data loss.
set(REFLECT84_COMPANY_NAME "Neon Foundry")
set(REFLECT84_PRODUCT_NAME "REFLECT-84")

# The panel's corner version stamp. Derived from project(... VERSION ...) rather than typed into
# the GUI, so the tag check in .github/workflows/release.yml governs what the panel says too.
set(REFLECT84_VERSION_SHORT "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

# The CMake target name and the product name are two separate axes and must not be confused: a
# CMake target cannot contain a hyphen, so the target is Reflect84 (and therefore the build
# directory is Reflect84_artefacts/) while every bundle on disk is REFLECT-84.vst3 and friends.
# neon-foundry/manifest/suite.toml carries the same distinction as cmake_target vs product_name.
#
# ICON_BIG/ICON_SMALL are the two sizes JUCE wants; it generates the .icns (and the Windows .ico)
# from them at build time, so there is no hand-maintained icon bundle in the repo. design/icon/
# holds a hand-tuned optical ramp rather than one artwork scaled - the 32/64px cuts drop the glow
# and reduce the step count, because bloom turns to mush below 48px (design/README.md, "Product
# icon"). JUCE only consumes these two, but the smaller cuts are kept as the source of truth for
# anywhere an icon is placed by hand.
juce_add_plugin(Reflect84
    PRODUCT_NAME "${REFLECT84_PRODUCT_NAME}"
    COMPANY_NAME "${REFLECT84_COMPANY_NAME}"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/design/icon/icon-1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/design/icon/icon-256.png"
    BUNDLE_ID "com.neonfoundry.reflect84"
    # **Without this the Standalone is silent and the meters look broken.** An app whose Info.plist
    # carries no NSMicrophoneUsageDescription is never granted capture access on macOS - the OS does
    # not prompt and does not error, the input stream simply delivers zeros. On an effect that reads
    # as dead metering: IN sits at its floor and OUT follows, because there is nothing to process.
    # The meters are wired correctly; they have nothing to show.
    #
    # Plugin formats are unaffected - the host owns the input there - so this only surfaces when
    # someone opens the Standalone to listen.
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "REFLECT-84 needs audio input so the Standalone can process what you play into it."
    PLUGIN_MANUFACTURER_CODE Nfdy
    PLUGIN_CODE Rf84
    FORMATS ${REFLECT84_FORMATS}
    VST3_CATEGORIES Fx Reverb
    AU_MAIN_TYPE kAudioUnitType_Effect
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    ${REFLECT84_EXTRA_ARGS})

target_sources(Reflect84 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/ProgramManager.cpp
    Source/DSP/PlateTank.cpp
    Source/DSP/DigitalRoomTank.cpp
    Source/DSP/FdnTank.cpp
    Source/DSP/ReverbEngine.cpp
    Source/GUI/ProgramHeader.cpp
    Source/GUI/ReflectLookAndFeel.cpp
    Source/GUI/PanelBackground.cpp
    Source/GUI/PanelReadouts.cpp
    Source/GUI/AlgorithmSwitch.cpp
    Source/GUI/TankScope.cpp
    Source/GUI/ReflectEditorContent.cpp)

target_compile_definitions(Reflect84 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    NF_COMPANY_NAME="${REFLECT84_COMPANY_NAME}"
    NF_PRODUCT_NAME="${REFLECT84_PRODUCT_NAME}"
    NF_VERSION_SHORT="${REFLECT84_VERSION_SHORT}")

target_link_libraries(Reflect84
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        Reflect84BinaryData
        nf::core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

add_subdirectory(Tests)
