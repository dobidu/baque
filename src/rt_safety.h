#pragma once
#include <juce_core/juce_core.h>

namespace RtSafety {
// Thread-local RT flag. Set true inside processBlock via ScopedAudioThreadGuard.
// Future: add jassert(!tl_is_audio_thread) inside operator new override to catch
// RT allocations — requires JUCE_CHECK_MEMORY_LEAKS=0 in the test target (CMakeLists).
inline thread_local bool tl_is_audio_thread = false;

struct ScopedAudioThreadGuard {
    ScopedAudioThreadGuard() noexcept { tl_is_audio_thread = true; }
    ~ScopedAudioThreadGuard() noexcept { tl_is_audio_thread = false; }
    JUCE_DECLARE_NON_COPYABLE(ScopedAudioThreadGuard)
};
} // namespace RtSafety
