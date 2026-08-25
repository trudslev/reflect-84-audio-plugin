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

project(Reflect84 VERSION 1.0.0 LANGUAGES CXX C)

# **MSVC needs /utf-8, and without it a non-ASCII literal is silently replaced by '?'.**
#
# MSVC's default execution charset is the system ANSI code page, so a `\u2212` in a narrow string
# literal has no representation and the compiler substitutes a question mark, warning C4566 and
# carrying on. Gatecrasher's first Windows test run emitted **168** of those warnings and failed two
# printed-scale marks: `\u2212` in a mark's numeral became `?`, `parseNumeral` read `?60` as 0, and
# the check correctly reported that a mark labelled 0 was drawn at the wrong angle.
#
# It is not a bad way of writing the character — a universal-character-name is the portable form,
# and the runtime `charToString ((juce_wchar) 0x2212)` used elsewhere in this suite is unaffected.
# It is the execution charset, and /utf-8 sets both source and execution to UTF-8.
#
# Nothing changes on macOS or Linux, where UTF-8 is already both.
if(MSVC)
    add_compile_options(/utf-8)
endif()

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
    GIT_TAG v1.0.0
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
        # **THE DELIVERED FILES, flat in `design/fonts/`, as of bundle 3 export 14.** Jost and
        # IBM Plex Mono were the two faces this casting sourced for itself under the old
        # `design/fonts/README.md` policy, in nested `jost/` and `ibm-plex-mono/` folders. Export
        # 14 delivers both, and its `fonts/ABSENT.md` states the directory's five files as the
        # complete set — so the self-fetched copies are superseded and are removed rather than
        # left beside them. They were DIFFERENT files, not just different paths: Plex 1033 glyphs
        # against the delivered 1028, Jost 102240 bytes against 61652.
        #
        # `IBMPlexMono-Medium.ttf` is gone with them. It was §2.2's selected corner label until
        # Barlow 700 arrived, and `ABSENT.md` records that no other weight of Plex is drawn here.
        # ABOUT-PART.md §2b's cursor. JUCE has no help cursor in StandardCursorType, so the
        # delivered asset is embedded and a MouseCursor is built from it.
        design/assets/about-cursor-2x.png
        design/fonts/Jost-Medium.ttf
        design/fonts/IBMPlexMono-Regular.ttf
        # The two shared faces, landed with design bundle 2 (2026-08-17). Share Tech Mono is the
        # suite's LCD face (HEADER-PART call 2) and Barlow Condensed SemiBold is panel lettering
        # (call 7). IBM Plex Mono STAYS: call 7 splits them, and this casting's printed numerals,
        # units and scope legends are its own mono. Adding these does not retire it.
        design/fonts/ShareTechMono-Regular.ttf
        # **THREE Barlow weights, because §2.2 encodes ALGORITHM's selection in weight AND value:**
        # 700 / #332b1e selected against 500 / #5e5440 unselected. Only SemiBold was delivered, so
        # Medium and Bold were fetched from the OFL source `design/fonts/README.md` names — the
        # same route it prescribes, and the Bold comes back byte-identical to the one the designers
        # delivered to Fifth Member, which is what confirms the source. See design-asks/OPEN.md.
        design/fonts/BarlowCondensed-Medium.ttf
        design/fonts/BarlowCondensed-SemiBold.ttf
        design/fonts/BarlowCondensed-Bold.ttf)

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
# **Local builds install into the user's plugin folders; CI must not.**
#
# A runner has no DAW to load them into, so the copy is pure waste there — but the reason it is an
# option rather than a tidy-up is the other direction. On a machine that DOES have those folders, a
# build overwrites what is installed, and this repository has already paid for that: a
# defect-causing run copied a deliberately broken AU over the user's working one, minutes after the
# fix was committed, with the tree clean and the remote up to date. Every signal said done.
#
# Defaults ON because the local loop depends on it — build, then load in the DAW. CI passes
# -DNF_COPY_PLUGIN_AFTER_BUILD=OFF.
option(NF_COPY_PLUGIN_AFTER_BUILD
       "Install the built plugin into the user's plugin folders after building" ON)

juce_add_plugin(Reflect84
    PRODUCT_NAME "${REFLECT84_PRODUCT_NAME}"
    COMPANY_NAME "${REFLECT84_COMPANY_NAME}"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/design/icons/reflect84-icon-1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/design/icons/reflect84-icon-256.png"
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
    COPY_PLUGIN_AFTER_BUILD ${NF_COPY_PLUGIN_AFTER_BUILD}
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
    Source/GUI/ReflectProgramList.cpp
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
    NF_VERSION_SHORT="${REFLECT84_VERSION_SHORT}"
    # ABOUT-PART.md §1: the box prints semver, which the major.minor short form cannot
    # carry. Both are defined because they are different strings for different places.
    NF_VERSION="${PROJECT_VERSION}")

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

# ---------------------------------------------------------------------------------------------
# THIRD-PARTY-LICENCES.txt — into every bundle's Contents/Resources
#
# **Embedding a font in the binary is redistribution**, and both licences these faces carry — OFL
# 1.1 and Apache 2.0 — require the copyright notice and licence text to travel with what is
# redistributed. The unit of redistribution is the PLUG-IN BUNDLE: someone receives a .vst3 on its
# own, so a licence file at the repo root discharges nothing for them. It has to be inside.
#
# The file is GENERATED from this CMakeLists' own juce_add_binary_data block by
# tools/generate_third_party_licences.py, so it cannot drift from what actually ships. Regenerate
# after adding or removing an embedded face; `--check` fails if any casting is stale.
#
# **macOS only, stated rather than silently true.** MACOSX_PACKAGE_LOCATION is what puts a file in
# a bundle, and on Windows and Linux the licence has to reach the user through the installer
# instead — see installer/ . Adding the .txt to target_sources off Apple would have CMake try to
# build it, which is why this is guarded rather than merely inert.
# **The path and its guard sit OUTSIDE the platform branch, and that is load-bearing.** Both used
# to live inside `if(APPLE)`, where the `set()` only executes on Apple — so every Windows and Linux
# build dereferenced an undefined variable and handed `copy_if_different` an empty source argument,
# and the missing-file FATAL_ERROR could not fire there either.
#
# It survived because nothing had ever built off Apple: all six castings' CI is macOS-first, three
# had never run at all, and a defect on a path nobody walks leaves no trace. Found while planning
# the release pipeline, proved with a minimal CMake reproduction rather than by reading.
set(NF_LICENCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/THIRD-PARTY-LICENCES.txt")
if(NOT EXISTS "${NF_LICENCE_FILE}")
    message(FATAL_ERROR
        "THIRD-PARTY-LICENCES.txt is missing. Run: python3 tools/generate_third_party_licences.py")
endif()

if(APPLE)
    set_source_files_properties("${NF_LICENCE_FILE}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    foreach(_nf_fmt IN ITEMS AU AUv3 VST3 Standalone)
        if(TARGET Reflect84_${_nf_fmt})
            target_sources(Reflect84_${_nf_fmt} PRIVATE "${NF_LICENCE_FILE}")
        endif()
    endforeach()
else()
    # **Windows and Linux: the VST3 is a bundle DIRECTORY too**, and it is the format that ships
    # there, so the licence belongs inside it for the same reason it does on macOS - the .vst3 is
    # what someone receives on its own. JUCE puts the binary at <bundle>/Contents/<arch>, so
    # Contents/Resources is one level up from TARGET_FILE_DIR.
    #
    # The Standalone off Apple is a bare executable with no bundle to put anything in; it reaches
    # the user through the installer or the tarball instead.
    if(TARGET Reflect84_VST3)
        add_custom_command(TARGET Reflect84_VST3 POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                    "$<TARGET_FILE_DIR:Reflect84_VST3>/../Resources"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${NF_LICENCE_FILE}"
                    "$<TARGET_FILE_DIR:Reflect84_VST3>/../Resources/THIRD-PARTY-LICENCES.txt"
            VERBATIM)
    endif()
endif()
