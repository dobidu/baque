cmake_minimum_required(VERSION 3.22)
project(BAQUE VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- JUCE 8.0.13 (pinned — não mude sem atualizar README e NOTICE) ---
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.13
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# --- SoundTouch v2.3.3 (R&D-TS fork, LGPL v2.1) — offline tempo-only stretch ---
FetchContent_Declare(
    SoundTouch
    GIT_REPOSITORY https://github.com/dobidu/baque-soundtouch.git
    GIT_TAG        v1.0.0-baque
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SoundTouch)

# --- Plugin ---
juce_add_plugin(BAQUE
    COMPANY_NAME                "BAQUE"
    BUNDLE_ID                   "br.ufpb.lavid.baque"
    PLUGIN_MANUFACTURER_CODE    "Lvd0"
    PLUGIN_CODE                 "Bqe1"
    PRODUCT_NAME                "BAQUE"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    FORMATS                     VST3 Standalone $<$<PLATFORM_ID:Darwin>:AU>
    VST3_CATEGORIES             "Instrument" "Drum"
)

# Flags de compilação recomendados pelo JUCE
target_compile_definitions(BAQUE
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

# Avisos de compilação no alvo BAQUE apenas (não nas deps JUCE)
if(MSVC)
    target_compile_options(BAQUE PRIVATE /W4)
else()
    target_compile_options(BAQUE PRIVATE -Wall -Wextra -Wpedantic)
endif()

juce_add_binary_data(BaqueAssets
    SOURCES assets/test_kick.wav
)

target_sources(BAQUE
    PRIVATE
        src/plugin_processor.cpp
        src/plugin_editor.cpp
        src/audio/sample_voice.cpp
        src/audio/voice_pool.cpp
        src/audio/scheduler.cpp
        src/audio/step_pattern.cpp
        src/audio/step_clock.cpp
        src/audio/sequencer.cpp
        src/audio/note_tracker.cpp
        src/audio/slicer.cpp
        src/audio/time_stretch.cpp
        src/audio/feel_engine.cpp
        src/audio/feel_presets.cpp
        src/audio/fx_chain.cpp
        src/audio/sidechain_comp.cpp
        src/audio/lo_fi_processor.cpp
        src/audio/granular_processor.cpp
)

target_link_libraries(BAQUE PRIVATE BaqueAssets)

target_include_directories(BAQUE PRIVATE src)

target_link_libraries(BAQUE
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        SoundTouch
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# --- Testes ---
enable_testing()
add_subdirectory(tests)
