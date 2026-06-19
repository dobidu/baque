#include "factory_preset_library.h"
#include "plugin_processor.h"
#include "audio/feel_presets.h"
#include "audio/step_pattern.h"

static constexpr const char* k_names[FactoryPresetLibrary::k_count] = {
    "Straight", "Boom-Bap", "Dilla Drunk", "Burial Broken", "FlyLo Wonk", "Bonobo Loose"
};

const char* FactoryPresetLibrary::name(int i) noexcept {
    jassert(i >= 0 && i < k_count);
    if (i < 0 || i >= k_count) return "";
    return k_names[i];
}

const char* FactoryPresetLibrary::category(int i) noexcept {
    jassert(i >= 0 && i < k_count);
    (void)i;
    return "factory";
}

void FactoryPresetLibrary::load_into(BaqueProcessor& proc, int i) noexcept {
    switch (i) {
        case 0: proc.set_feel_pattern(FeelPresets::straight());       break;
        case 1: proc.set_feel_pattern(FeelPresets::boom_bap());       break;
        case 2: proc.set_feel_pattern(FeelPresets::dilla_drunk());    break;
        case 3: proc.set_feel_pattern(FeelPresets::burial_broken());  break;
        case 4: proc.set_feel_pattern(FeelPresets::flylo_wonk());     break;
        case 5: proc.set_feel_pattern(FeelPresets::bonobo_loose());   break;
        default: jassert(false); return;
    }

    StepPattern pat{};
    for (int s : {0, 4, 8, 12}) {
        pat.set_active(0, s, true);
        pat.set_note(0, s, 36);
        pat.set_velocity(0, s, 100);
    }
    for (int s : {4, 12}) {
        pat.set_active(1, s, true);
        pat.set_note(1, s, 38);
        pat.set_velocity(1, s, 90);
    }
    proc.set_pattern(pat);

    if (auto* p = proc.getAPVTS().getParameter("master_gain"))
        p->setValueNotifyingHost(0.8f);
}
