#include "preset_manager.h"
#include "plugin_processor.h"

PresetManager::PresetManager(BaqueProcessor& proc) : proc_(proc) {}

juce::File PresetManager::user_preset_dir() {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("BAQUE/presets");
}

// Core write logic — called by both save() and save_to_file().
void PresetManager::save_to_file(const juce::String& name, const juce::String& category, const juce::File& dest) {
    juce::MemoryBlock data;
    proc_.getStateInformation(data);

    // Sanitize name — strip path separators; fallback to "unnamed"
    const juce::String safe = name.replaceCharacters("/\\:*?\"<>|", "_________");

    // Format: 4-byte magic 0x42515031 ("BQP1") + 4-byte metadata length + metadata XML + binary state
    juce::MemoryBlock out;
    juce::MemoryOutputStream os(out, false);

    const juce::String meta_xml = "<preset name=\"" + safe + "\" category=\"" + category + "\"/>";
    const juce::MemoryBlock meta(meta_xml.toRawUTF8(), static_cast<size_t>(meta_xml.getNumBytesAsUTF8()));

    const uint32_t magic    = juce::ByteOrder::swapIfBigEndian(static_cast<uint32_t>(0x42515031u));
    const uint32_t meta_len = juce::ByteOrder::swapIfBigEndian(static_cast<uint32_t>(meta.getSize()));
    os.write(&magic,    4);
    os.write(&meta_len, 4);
    os.write(meta.getData(), meta.getSize());
    os.write(data.getData(), data.getSize());

    dest.replaceWithData(out.getData(), out.getSize());
}

void PresetManager::save(const juce::String& name, const juce::String& category) {
    const juce::File dir = user_preset_dir();
    if (!dir.exists())
        dir.createDirectory();

    // Sanitize name for filename
    const juce::String safe = name.replaceCharacters("/\\:*?\"<>|", "_________");
    const juce::File dest   = dir.getChildFile((safe.isEmpty() ? "unnamed" : safe) + k_extension);
    save_to_file(name, category, dest);
}

void PresetManager::load(const juce::File& file) {
    if (!file.existsAsFile()) return;

    juce::MemoryBlock raw;
    file.loadFileAsData(raw);
    if (raw.getSize() < 8) return;

    // Parse header: magic + meta_len + meta + binary state
    const auto* bytes = static_cast<const uint8_t*>(raw.getData());
    uint32_t magic = 0;
    std::memcpy(&magic, bytes, 4);
    magic = juce::ByteOrder::swapIfBigEndian(magic);

    const uint8_t* state_ptr = bytes;
    size_t         state_size = raw.getSize();

    if (magic == 0x42515031u) {
        uint32_t meta_len = 0;
        std::memcpy(&meta_len, bytes + 4, 4);
        meta_len             = juce::ByteOrder::swapIfBigEndian(meta_len);
        const size_t offset  = 8 + static_cast<size_t>(meta_len);
        if (offset >= raw.getSize()) return;
        state_ptr  = bytes + offset;
        state_size = raw.getSize() - offset;
    }
    // If magic doesn't match, treat entire file as raw state (legacy/naked blobs)

    proc_.setStateInformation(state_ptr, static_cast<int>(state_size));
}

juce::Array<PresetManager::PresetInfo> PresetManager::list_user_presets() const {
    juce::Array<PresetInfo> result;
    const juce::File dir = user_preset_dir();
    if (!dir.isDirectory()) return result;

    juce::Array<juce::File> files;
    dir.findChildFiles(files, juce::File::findFiles, false, "*" + juce::String(k_extension));
    files.sort();

    for (const auto& f : files) {
        PresetInfo info;
        info.file     = f;
        info.name     = f.getFileNameWithoutExtension();
        info.category = "custom"; // metadata parsing deferred to Phase 11-02 browser
        result.add(info);
    }
    return result;
}
