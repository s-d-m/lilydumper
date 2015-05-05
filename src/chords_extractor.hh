#ifndef CHORDS_EXTRACTOR_HH_
#define CHORDS_EXTRACTOR_HH_

#include <vector>
#include "notes_file_extractor.hh"

struct chord_t
{
    chord_t()
      : notes()
    {
    }

    // a chord are just notes played at the same time
    std::vector<note_t> notes;
};

// group notes into chords
std::vector<chord_t> get_chords(const std::vector<note_t>& notes);

#endif /* CHORDS_EXTRACTOR_HH_ */
