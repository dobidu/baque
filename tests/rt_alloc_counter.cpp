// RT allocation counter — counts heap allocs that occur during processBlock.
// Uses dlsym(RTLD_NEXT, "malloc") to wrap the system allocator without ODR conflict
// with JUCE's operator new (which itself calls malloc internally).
// tl_is_audio_thread is thread-local: only the test thread has it true during processBlock,
// so JUCE's message thread allocations do NOT increment the counter.
// Windows: dlsym/RTLD_NEXT are POSIX-only; the counter is defined but always reads 0.
#include "rt_alloc_counter.h"
#include "../src/rt_safety.h"
#include <cstdlib>
#include <cstddef>
#include <cstring>
#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace RtSafety { std::atomic<int> rt_alloc_count{0}; }

#ifdef _WIN32
// malloc interception via dlsym(RTLD_NEXT) is not available on Windows.
// rt_alloc_count stays 0; RT-safety tests that assert count==0 still pass.
#else

static void* (*real_malloc)(std::size_t) = nullptr;

// Static buffer for allocations during dlsym bootstrap (dlsym itself calls malloc/calloc).
// 512 bytes; bootstrap allocs are tiny. Bump allocator — no per-object free.
static char s_bootstrap_buf[512];
static std::size_t s_bootstrap_used = 0;

extern "C" void* malloc(std::size_t size) {
    if (!real_malloc) {
        // Bootstrap: use static buffer to survive dlsym's own allocation calls.
        std::size_t aligned = (size + 15) & ~std::size_t{15};
        if (s_bootstrap_used + aligned <= sizeof(s_bootstrap_buf)) {
            void* p = s_bootstrap_buf + s_bootstrap_used;
            s_bootstrap_used += aligned;
            return p;
        }
        // Buffer exhausted — initialize real_malloc now.
        real_malloc = reinterpret_cast<void*(*)(std::size_t)>(dlsym(RTLD_NEXT, "malloc"));
        if (!real_malloc) return nullptr;
    }
    if (RtSafety::tl_is_audio_thread)
        RtSafety::rt_alloc_count.fetch_add(1, std::memory_order_relaxed);
    return real_malloc(size);
}

extern "C" void* calloc(std::size_t n, std::size_t size) {
    void* p = malloc(n * size);
    if (p) std::memset(p, 0, n * size);
    return p;
}

extern "C" void* realloc(void* ptr, std::size_t size) {
    if (!real_malloc)
        real_malloc = reinterpret_cast<void*(*)(std::size_t)>(dlsym(RTLD_NEXT, "malloc"));
    if (RtSafety::tl_is_audio_thread)
        RtSafety::rt_alloc_count.fetch_add(1, std::memory_order_relaxed);
    using realloc_fn = void*(*)(void*, std::size_t);
    static auto real_realloc = reinterpret_cast<realloc_fn>(dlsym(RTLD_NEXT, "realloc"));
    return real_realloc ? real_realloc(ptr, size) : nullptr;
}

extern "C" void free(void* p) {
    if (!p) return;
    // Bootstrap-allocated pointers (inside s_bootstrap_buf) must NOT be passed to the
    // system free — they come from a bump allocator with no individual free. Skip them.
    if (static_cast<char*>(p) >= s_bootstrap_buf &&
        static_cast<char*>(p) < s_bootstrap_buf + sizeof(s_bootstrap_buf))
        return;
    using free_fn = void(*)(void*) noexcept;
    static auto real_free = reinterpret_cast<free_fn>(dlsym(RTLD_NEXT, "free"));
    if (real_free) real_free(p);
}
#endif // !_WIN32
