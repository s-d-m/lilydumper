#ifndef CURSOR_BOXES_EXTRACTOR_HH_
#define CURSOR_BOXES_EXTRACTOR_HH_

#include <vector>

#include "chords_extractor.hh" // for note_t definition
#include "svg_extractor.hh" // for svg_file_t definition


struct cursor_box_t
{
    uint32_t left;
    uint32_t right;
    uint32_t top;
    uint32_t bottom;
    uint64_t start_time; // at which point this cursor must appear
    uint16_t svg_file_pos; // the position of the svg file this cursor_box relates to
    uint8_t  system_number; // the system in the svg file this cursor covers.
};

std::vector<cursor_box_t> get_cursor_boxes(const std::vector<chord_t>& chords,
					   const std::vector<svg_file_t>& svg_files);

#endif /* CURSOR_BOXES_EXTRACTOR_HH_ */
