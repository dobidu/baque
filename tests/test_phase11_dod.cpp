#include "../src/plugin_processor.h"
#include "../src/preset_manager.h"
#include "../src/factory_preset_library.h"
#include "../src/audio/feel_pattern.h"
#include "../src/audio/plock_pattern.h"
#include <BinaryData.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <juce_core/juce_core.h>

TEST_CASE("P11D1 - StepPattern round-trips through getState/setState", "[p11_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;

    StepPattern pat;
    pat.set_active(0, 0, false);
    pat.set_active(1, 3, true);
    pat.set_note(1, 3, 48);
    pat.set_velocity(1, 3, 80);
    proc.set_pattern(pat);

    juce::MemoryBlock data;
    proc.getStateInformation(data);
    proc.setStateInformation(data.getData(), static_cast<int>(data.getSize()));

    const StepPattern restored = proc.current_pattern();
    CHECK_FALSE(restored.is_active(0, 0));
    CHECK(restored.is_active(1, 3));
    CHECK(restored.get_note(1, 3) == 48);
    CHECK(restored.get_velocity(1, 3) == 80);
}

TEST_CASE("P11D2 - FeelPattern round-trips through getState/setState", "[p11_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;

    // Set non-default FeelPattern via set_feel_pattern() — analogous to set_pattern() from 05-01
    FeelPattern fp;
    fp.enabled = true;
    fp.steps[0].timing_ms = 25.5f;
    fp.steps[3].vel_scale = 0.7f;
    fp.seed = 42u;
    proc.set_feel_pattern(fp);

    juce::MemoryBlock data;
    proc.getStateInformation(data);
    proc.setStateInformation(data.getData(), static_cast<int>(data.getSize()));

    const FeelPattern restored = proc.current_feel_pattern();
    CHECK(restored.enabled);
    CHECK_THAT(restored.steps[0].timing_ms, Catch::Matchers::WithinAbs(25.5f, 0.001f));
    CHECK_THAT(restored.steps[3].vel_scale,  Catch::Matchers::WithinAbs(0.7f,  0.001f));
    CHECK(restored.seed == 42u);
}

TEST_CASE("P11D3 - PresetManager saves and loads a preset", "[p11_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    PresetManager pm(proc);

    // Set a non-default APVTS param
    if (auto* param = proc.getAPVTS().getParameter("master_gain"))
        param->setValueNotifyingHost(0.3f);

    // Write directly to a TemporaryFile — never touches user_preset_dir()
    juce::TemporaryFile tmp_preset(".bqpreset");
    pm.save_to_file("test_groove", "custom", tmp_preset.getFile());
    REQUIRE(tmp_preset.getFile().existsAsFile());

    // Load into fresh processor and verify full round-trip
    BaqueProcessor proc2;
    PresetManager pm2(proc2);
    pm2.load(tmp_preset.getFile());

    auto* p = proc2.getAPVTS().getParameter("master_gain");
    REQUIRE(p != nullptr);
    CHECK_THAT(p->getValue(), Catch::Matchers::WithinAbs(0.3f, 0.01f));
}

TEST_CASE("P11D4 - SamplePad source_file_ round-trips through state", "[p11_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;

    // Write embedded test_kick.wav to a temp file
    juce::TemporaryFile tmp(".wav");
    {
        juce::FileOutputStream os(tmp.getFile());
        os.write(BinaryData::test_kick_wav, BinaryData::test_kick_wavSize);
    }

    proc.load_sample_from_file(2, tmp.getFile());
    CHECK(proc.current_pad(2).num_samples() > 0);
    CHECK(proc.current_pad(2).source_file() == tmp.getFile());

    // Round-trip — file must still exist for reload to work (TemporaryFile is alive)
    juce::MemoryBlock data;
    proc.getStateInformation(data);

    BaqueProcessor proc2;
    proc2.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
    CHECK(proc2.current_pad(2).source_file() == tmp.getFile());
    CHECK(proc2.current_pad(2).num_samples() > 0);
}

TEST_CASE("P11D5 - v4 state blob (no v5 subtrees) loads without crash", "[p11_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;

    // Get valid v5 state, then strip v5 subtrees to simulate a v4 blob
    juce::MemoryBlock data;
    proc.getStateInformation(data);

    auto state = juce::ValueTree::readFromData(data.getData(), data.getSize());
    for (const char* nm : {"pattern_v5", "feel_v5", "plock_v5", "pads_v5"}) {
        auto child = state.getChildWithName(nm);
        if (child.isValid())
            state.removeChild(child, nullptr);
    }

    juce::MemoryBlock v4_data;
    {
        juce::MemoryOutputStream os(v4_data, false);
        state.writeToStream(os);
    }

    BaqueProcessor proc2;
    REQUIRE_NOTHROW(proc2.setStateInformation(v4_data.getData(), static_cast<int>(v4_data.getSize())));
    CHECK_FALSE(proc2.current_feel_pattern().enabled); // defaults to disabled
    CHECK(proc2.current_pad(0).source_file() == juce::File{}); // no path — empty default
}

TEST_CASE("P11D6: FactoryPresetLibrary loads correctly", "[p11_dod]") {
    using Catch::Matchers::WithinAbs;
    // Dilla Drunk (index 2) — primary coverage
    {
        juce::ScopedJuceInitialiser_GUI init;
        BaqueProcessor proc;
        FactoryPresetLibrary::load_into(proc, 2);
        const auto fp = proc.current_feel_pattern();
        REQUIRE(fp.enabled);
        CHECK(fp.seed == 313u);
        CHECK_THAT(fp.steps[1].timing_ms, WithinAbs(20.0f, 0.01f));
        CHECK_THAT(fp.steps[4].timing_ms, WithinAbs(25.0f, 0.01f));
        const auto pat = proc.current_pattern();
        CHECK(pat.is_active(0, 0));  // kick step 0
        CHECK(pat.is_active(1, 4));  // snare step 4
    }
    // Straight (index 0) — verifies switch case 0, enabled flag, zero timing offsets
    {
        juce::ScopedJuceInitialiser_GUI init;
        BaqueProcessor proc;
        FactoryPresetLibrary::load_into(proc, 0);
        const auto fp = proc.current_feel_pattern();
        CHECK(fp.enabled);
        CHECK(fp.seed == 1u);
        CHECK_THAT(fp.steps[0].timing_ms, WithinAbs(0.0f, 0.01f));
    }
    // Valid boundary: k_count==6; name(0) and name(5) are non-empty
    // Do NOT call name(6) — fires jassert in debug builds, crashes test process
    CHECK(FactoryPresetLibrary::k_count == 6);
    CHECK(!juce::String(FactoryPresetLibrary::name(0)).isEmpty());
    CHECK(!juce::String(FactoryPresetLibrary::name(5)).isEmpty());
}

TEST_CASE("P11D7: factory preset round-trips via file", "[p11_dod]") {
    using Catch::Matchers::WithinAbs;
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc1;
    FactoryPresetLibrary::load_into(proc1, 2); // Dilla Drunk
    juce::TemporaryFile tmp(".bqpreset");
    PresetManager pm1{proc1};
    pm1.save_to_file("Dilla Drunk", "factory", tmp.getFile());
    REQUIRE(tmp.getFile().existsAsFile());

    BaqueProcessor proc2;
    PresetManager pm2{proc2};
    pm2.load(tmp.getFile());
    const auto fp = proc2.current_feel_pattern();
    REQUIRE(fp.enabled);
    CHECK(fp.seed == 313u);
    CHECK_THAT(fp.steps[1].timing_ms, WithinAbs(20.0f, 0.01f));
}
