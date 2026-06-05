#pragma once

// Estado de transporte do host — POD, sem dependências JUCE.
// Definido aqui para evitar dependência circular entre plugin_processor.h e sequencer.h.
struct TransportState {
    bool is_playing = false;
    double bpm = 120.0;
    double ppq_position = 0.0; // posição em quarter notes
    double sample_rate = 44100.0;
};
