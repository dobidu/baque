cmake_minimum_required(VERSION 3.22)
project(BAQUE VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- JUCE 8.0.13 (pinned — não mude sem atualizar README e NOTICE) ---
include(FetchContent)

# Route external dep install() rules to ExternalDeps component so CPack excludes them
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "ExternalDeps")

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

# Reset — our own install() calls specify COMPONENT explicitly
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "Unspecified")

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
    SOURCES
        assets/test_kick.wav
        assets/fonts/SpaceGrotesk-Medium.ttf
        assets/fonts/JetBrainsMono-Regular.ttf
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
        src/audio/scatter_engine.cpp
        src/audio/tape_stop_processor.cpp
        src/audio/gater_processor.cpp
        src/audio/midi_clock.cpp
        src/ui/baque_look_and_feel.cpp
        src/ui/baque_meter.cpp
        src/ui/header_component.cpp
        src/ui/pad_grid_component.cpp
        src/ui/sequencer_component.cpp
        src/ui/sample_editor_component.cpp
        src/ui/feel_field_component.cpp
        src/ui/perform_screen.cpp
        src/ui/fx_screen.cpp
        src/ui/mix_screen.cpp
        src/ui/perf_fx_screen.cpp
        src/ui/midi_screen.cpp
        src/ui/browser_screen.cpp
        src/preset_manager.cpp
        src/factory_preset_library.cpp
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

# --- Install rules (CPack packaging — Plan 13-02) ---
# Run after Release build: cmake --install build --prefix <dir> --config Release
install(DIRECTORY "${CMAKE_BINARY_DIR}/BAQUE_artefacts/Release/VST3/BAQUE.vst3"
        DESTINATION "VST3"
        COMPONENT VST3)

if(WIN32)
    install(PROGRAMS "${CMAKE_BINARY_DIR}/BAQUE_artefacts/Release/Standalone/BAQUE.exe"
            DESTINATION "Standalone"
            COMPONENT Standalone)
elseif(APPLE)
    install(DIRECTORY "${CMAKE_BINARY_DIR}/BAQUE_artefacts/Release/Standalone/BAQUE.app"
            DESTINATION "Standalone"
            COMPONENT Standalone)
else()
    install(PROGRAMS "${CMAKE_BINARY_DIR}/BAQUE_artefacts/Release/Standalone/BAQUE"
            DESTINATION "Standalone"
            COMPONENT Standalone)
endif()

if(APPLE)
    # install(DIRECTORY) has no OPTIONAL flag — use CODE to guard with if(EXISTS).
    # AU may not be present on CI runners without Apple Developer credentials.
    install(CODE "
        set(_au \"${CMAKE_BINARY_DIR}/BAQUE_artefacts/Release/AU/BAQUE.component\")
        if(EXISTS \"\${_au}\")
            file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/AU\" TYPE DIRECTORY FILES \"\${_au}\")
        else()
            message(STATUS \"AU component not found — skipping AU packaging\")
        endif()
    " COMPONENT AU)
endif()

# --- CPack (release packaging — Plan 13-02) ---
# Produces: BAQUE-{VERSION}-{OS}.zip via `cpack -G ZIP` in build dir
set(CPACK_PACKAGE_NAME "BAQUE")
set(CPACK_PACKAGE_VENDOR "LAVID/UFPB")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hybrid groovebox audio plugin — VST3/AU/Standalone")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "BAQUE-${PROJECT_VERSION}-Windows")
elseif(APPLE)
    set(CPACK_PACKAGE_FILE_NAME "BAQUE-${PROJECT_VERSION}-macOS")
else()
    set(CPACK_PACKAGE_FILE_NAME "BAQUE-${PROJECT_VERSION}-Linux")
endif()
set(CPACK_GENERATOR "ZIP")
set(CPACK_SOURCE_GENERATOR "")
set(CPACK_STRIP_FILES TRUE)

# Component-aware packaging: only VST3/Standalone(/AU) go into the ZIP.
# ExternalDeps component (JUCE headers, SoundTouch cmake files) is excluded.
# CPACK_ARCHIVE_COMPONENT_INSTALL must be ON for CPACK_COMPONENTS_ALL to filter the archive.
# CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE produces a single ZIP (not one per component).
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
if(APPLE)
    set(CPACK_COMPONENTS_ALL VST3 Standalone AU)
else()
    set(CPACK_COMPONENTS_ALL VST3 Standalone)
endif()

# Skip preinstall rebuild — artifacts must be built before running cpack
# Prevents clock-skew spurious rebuilds on WSL2 and avoids rebuilding tests on packaging
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)

include(CPack)
