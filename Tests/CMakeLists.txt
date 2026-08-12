juce_add_console_app(Reflect84Tests PRODUCT_NAME "Reflect84Tests")

target_sources(Reflect84Tests PRIVATE
    TestMain.cpp
    ReadoutConformanceTests.cpp
    GrainSpecTests.cpp
    FactoryProgramsTests.cpp
    ProgramManagerTests.cpp
    DisplayBudgetTests.cpp
    ReverbEngineTests.cpp
    ../Source/DSP/ProgramManager.cpp
    ../Source/DSP/PlateTank.cpp
    ../Source/DSP/DigitalRoomTank.cpp
    ../Source/DSP/FdnTank.cpp
    ../Source/DSP/ReverbEngine.cpp)

# NF_COMPANY_NAME/NF_PRODUCT_NAME come from the parent CMakeLists and must match what the plugin
# target gets - ProgramManager.cpp is compiled into both, and builds the user-Programs path from
# them. Inherited from the parent scope rather than restated here, so there is nothing to re-sync.
target_compile_definitions(Reflect84Tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    NF_COMPANY_NAME="${REFLECT84_COMPANY_NAME}"
    NF_PRODUCT_NAME="${REFLECT84_PRODUCT_NAME}")

# Reflect84BinaryData, because DisplayBudgetTests measures against ReflectTheme.h and the theme
# header includes BinaryData.h for the embedded faces. Linking the real generated header rather
# than stubbing it is the point: the font a test measures must be the font the panel draws.
target_link_libraries(Reflect84Tests PRIVATE
    Reflect84BinaryData
    # ProgramManager.cpp is compiled into both targets and now calls into core, so both link it.
    nf::core
    juce::juce_dsp
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
