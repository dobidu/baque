include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.15.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(baque_tests test_smoke.cpp test_voice.cpp test_transport.cpp test_sequencer.cpp test_pad_bank.cpp)

target_link_libraries(baque_tests
    PRIVATE
        Catch2::Catch2WithMain
        BAQUE
        BaqueAssets
        juce::juce_audio_processors
        juce::juce_audio_utils
)

# Acesso aos headers do plugin sem depender do alvo GUI
target_include_directories(baque_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

include(CTest)
include(Catch)
# PRE_TEST: descoberta em tempo de ctest, não de build (evita problema de ordem no WSL2)
catch_discover_tests(baque_tests DISCOVERY_MODE PRE_TEST)
