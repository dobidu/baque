#pragma once
class BaqueProcessor;

class FactoryPresetLibrary {
  public:
    static constexpr int k_count = 6;

    [[nodiscard]] static const char* name(int i) noexcept;
    [[nodiscard]] static const char* category(int i) noexcept;
    static void load_into(BaqueProcessor& proc, int i) noexcept;

    FactoryPresetLibrary() = delete;
};
