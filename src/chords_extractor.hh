#ifndef CHORDS_EXTRACTOR_HH_
#define CHORDS_EXTRACTOR_HH_

#include <vector>
#include "utils.hh"

// group notes into chords
std::vector<chord_t> get_chords(const std::vector<note_t>& notes);

#endif /* CHORDS_EXTRACTOR_HH_ */
