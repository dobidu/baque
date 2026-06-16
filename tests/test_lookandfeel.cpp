#include "audio/ui_state_snapshot.h"
#include "ui/baque_look_and_feel.h"
#include "ui/baque_meter.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("LAF1 - role colour constants match spec", "[laf]") {
    CHECK(BaqueLookAndFeel::k_ember == 0xffe8502eu);
    CHECK(BaqueLookAndFeel::k_ocre == 0xffe6b34du);
    CHECK(BaqueLookAndFeel::k_clay == 0xffd98a4bu);
    CHECK(BaqueLookAndFeel::k_bone == 0xfff2e9ddu);
    CHECK(BaqueLookAndFeel::k_indigo == 0xff5b86a8u);

    CHECK(BaqueLookAndFeel::ember().getARGB() == 0xffe8502eu);
    CHECK(BaqueLookAndFeel::ocre().getARGB() == 0xffe6b34du);
    CHECK(BaqueLookAndFeel::clay().getARGB() == 0xffd98a4bu);
    CHECK(BaqueLookAndFeel::bone().getARGB() == 0xfff2e9ddu);
    CHECK(BaqueLookAndFeel::indigo().getARGB() == 0xff5b86a8u);
}

TEST_CASE("LAF2 - all four themes construct; backgrounds differ", "[laf]") {
    BaqueLookAndFeel barro(BaqueLookAndFeel::Theme::barro);
    BaqueLookAndFeel cinza(BaqueLookAndFeel::Theme::cinza);
    BaqueLookAndFeel maracatu(BaqueLookAndFeel::Theme::maracatu);
    BaqueLookAndFeel papel(BaqueLookAndFeel::Theme::papel);

    CHECK(barro.theme() == BaqueLookAndFeel::Theme::barro);
    CHECK(cinza.theme() == BaqueLookAndFeel::Theme::cinza);
    CHECK(maracatu.theme() == BaqueLookAndFeel::Theme::maracatu);
    CHECK(papel.theme() == BaqueLookAndFeel::Theme::papel);

    // all four backgrounds must be distinct
    const auto bg_b = barro.background().getARGB();
    const auto bg_c = cinza.background().getARGB();
    const auto bg_m = maracatu.background().getARGB();
    const auto bg_p = papel.background().getARGB();

    CHECK(bg_b != bg_c);
    CHECK(bg_b != bg_m);
    CHECK(bg_b != bg_p);
    CHECK(bg_c != bg_m);
    CHECK(bg_c != bg_p);
    CHECK(bg_m != bg_p);
}

TEST_CASE("LAF3 - static_assert on role colour values", "[laf]") {
    static_assert(BaqueLookAndFeel::k_ember == 0xffe8502eu, "ember mismatch");
    static_assert(BaqueLookAndFeel::k_ocre == 0xffe6b34du, "ocre mismatch");
    static_assert(BaqueLookAndFeel::k_clay == 0xffd98a4bu, "clay mismatch");
    static_assert(BaqueLookAndFeel::k_bone == 0xfff2e9ddu, "bone mismatch");
    static_assert(BaqueLookAndFeel::k_indigo == 0xff5b86a8u, "indigo mismatch");
    SUCCEED("static_assert compile-time checks passed");
}

TEST_CASE("LAF4 - BaqueMeter timer not running when component is not visible", "[laf][meter]") {
    UiStateSnapshot snapshot;
    BaqueMeter meter(snapshot);
    // Freshly constructed meter is not visible — timer must not be running.
    CHECK_FALSE(meter.isTimerRunning());
}
