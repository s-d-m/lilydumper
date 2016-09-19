#ifndef NOTES_FILE_EXTRACTOR_HH_
#define NOTES_FILE_EXTRACTOR_HH_

#include <vector>
#include "utils.hh"

std::vector<note_t> get_notes(const fs::path& filename);

#endif /* NOTES_FILE_EXTRACTOR_HH_ */
