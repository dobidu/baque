#include "note_tracker.h"

void NoteTracker::note_triggered(int lane, uint8_t note) noexcept {
    active_notes_[static_cast<std::size_t>(lane)] = note;
}

uint8_t NoteTracker::get_active_note(int lane) const noexcept {
    return active_notes_[static_cast<std::size_t>(lane)];
}

void NoteTracker::reset() noexcept {
    active_notes_.fill(0);
}
