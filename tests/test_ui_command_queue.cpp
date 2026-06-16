#include <catch2/catch_test_macros.hpp>

#include "audio/ui_command_queue.h"

// QC1: FIFO order preserved across AbstractFifo wrap-around.
// Total pushed > k_capacity; any ring-wrap bug corrupts ordering after the first cycle.
TEST_CASE("QC1 FIFO order preserved across wrap-around", "[queue]") {
    UiCommandQueue q;
    constexpr int total = UiCommandQueue::k_capacity * 2 + 7;
    int expected = 0;
    for (int i = 0; i < total; ++i) {
        UiCommand cmd;
        cmd.type = UiCommandType::set_mute;
        cmd.a    = i;
        REQUIRE(q.push(cmd));
        q.drain([&](const UiCommand& c) {
            REQUIRE(c.a == expected++);
        });
    }
    REQUIRE(expected == total);
}

// QC2: push on full queue returns false; existing contents remain intact and in order.
TEST_CASE("QC2 push on full queue returns false contents intact", "[queue]") {
    UiCommandQueue q;
    for (int i = 0; i < UiCommandQueue::k_capacity; ++i) {
        UiCommand cmd;
        cmd.type = UiCommandType::set_solo;
        cmd.a    = i;
        REQUIRE(q.push(cmd));
    }
    UiCommand extra;
    extra.type = UiCommandType::set_solo;
    extra.a    = -1; // sentinel: must never appear in drain
    REQUIRE_FALSE(q.push(extra));

    int count = 0;
    q.drain([&](const UiCommand& c) {
        REQUIRE(c.a == count++);
    });
    REQUIRE(count == UiCommandQueue::k_capacity);
}

// QC3: drain on empty queue calls apply zero times.
TEST_CASE("QC3 drain on empty queue calls apply zero times", "[queue]") {
    UiCommandQueue q;
    int calls = 0;
    q.drain([&](const UiCommand&) { ++calls; });
    REQUIRE(calls == 0);
}

// QC4: interleaved push/drain across multiple cycles preserves FIFO order.
TEST_CASE("QC4 interleaved push drain preserves order", "[queue]") {
    UiCommandQueue q;
    int seq_in = 0, seq_out = 0;

    auto push_n = [&](int n) {
        for (int i = 0; i < n; ++i) {
            UiCommand cmd;
            cmd.type = UiCommandType::set_step;
            cmd.a    = seq_in++;
            REQUIRE(q.push(cmd));
        }
    };
    auto drain_all = [&]() {
        q.drain([&](const UiCommand& c) {
            REQUIRE(c.a == seq_out++);
        });
    };

    push_n(10); drain_all();  // 10 through
    push_n(5);  push_n(8);   drain_all(); // 13 through
    push_n(3);  drain_all();  // 3 through
    push_n(20); drain_all();  // 20 through
    push_n(1);  drain_all();  // 1 through

    REQUIRE(seq_in == seq_out);
    REQUIRE(seq_in == 47);
}
