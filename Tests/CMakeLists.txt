juce_add_console_app(Reflect84Tests PRODUCT_NAME "Reflect84Tests")

target_sources(Reflect84Tests PRIVATE
    RealtimeSafetyTests.cpp
    NumericalRobustnessTests.cpp
    LockContentionTests.cpp
    InvarianceTests.cpp
    TestMain.cpp
    PrintedScaleTests.cpp
    ReadoutConformanceTests.cpp
    GrainSpecTests.cpp
    FactoryProgramsTests.cpp
    ProgramManagerTests.cpp
    DisplayBudgetTests.cpp
    ReverbEngineTests.cpp
    EditorWiringTests.cpp
    ../Source/DSP/ProgramManager.cpp
    ../Source/DSP/PlateTank.cpp
    ../Source/DSP/DigitalRoomTank.cpp
    ../Source/DSP/FdnTank.cpp
    ../Source/DSP/ReverbEngine.cpp
    # **The REAL processor and the REAL editor, not stand-ins.** Everything above this line is what
    # core does not own; everything below it is the wiring, which is the part no test could reach
    # while the target compiled DSP alone.
    #
    # That gap is not theoretical here. This casting shipped the stale-replay guard with ZERO call
    # sites for its disarm, and nothing in this suite could have seen it: the defect was in
    # ReflectEditorContent.cpp, which was not compiled into anything a test could run.
    ../Source/PluginProcessor.cpp
    ../Source/PluginEditor.cpp
    ../Source/GUI/ReflectEditorContent.cpp
    ../Source/GUI/ProgramHeader.cpp
    ../Source/GUI/ReflectProgramList.cpp
    ../Source/GUI/ReflectLookAndFeel.cpp
    ../Source/GUI/AlgorithmSwitch.cpp
    ../Source/GUI/PanelBackground.cpp
    ../Source/GUI/PanelReadouts.cpp
    ../Source/GUI/TankScope.cpp)

# NF_COMPANY_NAME/NF_PRODUCT_NAME come from the parent CMakeLists and must match what the plugin
# target gets - ProgramManager.cpp is compiled into both, and builds the user-Programs path from
# them. Inherited from the parent scope rather than restated here, so there is nothing to re-sync.
#
# NF_VERSION_SHORT arrived with the GUI sources: PanelBackground.cpp prints the version stamp from
# it, and a console app gets none of the plugin target's definitions. Inherited from the same parent
# variable the plugin uses, for the same reason as the two above — the panel a test measures must
# carry the string the panel draws.
target_compile_definitions(Reflect84Tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    NF_COMPANY_NAME="${REFLECT84_COMPANY_NAME}"
    NF_PRODUCT_NAME="${REFLECT84_PRODUCT_NAME}"
    NF_VERSION_SHORT="${REFLECT84_VERSION_SHORT}")

# Reflect84BinaryData, because DisplayBudgetTests measures against ReflectTheme.h and the theme
# header includes BinaryData.h for the embedded faces. Linking the real generated header rather
# than stubbing it is the point: the font a test measures must be the font the panel draws.
target_link_libraries(Reflect84Tests PRIVATE
    # The bug sweep's measurement drivers. TEST target only — never the plugin, which must not
    # carry an operator new override. See nf/testing/ProcessorHarness.h.
    nf::testing
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
