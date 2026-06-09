include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.15.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(baque_tests test_smoke.cpp test_voice.cpp test_transport.cpp test_sequencer.cpp test_pad_bank.cpp test_slicer.cpp test_time_stretch.cpp test_feel_engine.cpp test_feel_presets.cpp test_plock.cpp test_fx_chain.cpp test_sidechain_comp.cpp test_fx_plock_dod.cpp test_lo_fi.cpp test_lo_fi_chain.cpp test_granular.cpp test_granular_chain.cpp test_scatter.cpp test_scatter_chain.cpp test_tape_stop.cpp test_gater.cpp test_trig_fills.cpp test_mute_solo.cpp test_phase8_dod.cpp test_midi_routing.cpp test_midi_clock.cpp)

target_link_libraries(baque_tests
    PRIVATE
        Catch2::Catch2WithMain
        BAQUE
        BaqueAssets
        SoundTouch
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
)

# Acesso aos headers do plugin sem depender do alvo GUI
target_include_directories(baque_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

include(CTest)
include(Catch)
# PRE_TEST: descoberta em tempo de ctest, não de build (evita problema de ordem no WSL2)
catch_discover_tests(baque_tests DISCOVERY_MODE PRE_TEST)
