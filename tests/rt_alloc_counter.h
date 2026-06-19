#pragma once
#include <atomic>

namespace RtSafety {
    // Incremented by malloc override when tl_is_audio_thread == true.
    // Defined in tests/rt_alloc_counter.cpp (test binary only — not in plugin).
    extern std::atomic<int> rt_alloc_count;
}
