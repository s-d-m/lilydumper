#ifndef UTILS_HH_
#define UTILS_HH_

#include <string>
#include <vector>
#include "notes_file_extractor.hh" // for note_t definition
#include "keyboard_events_extractor.hh" // for key_event definition

std::string get_value_from_field(const std::string& id_str, const char* const field);

void debug_dump(const std::vector<key_event>& song, const char* const out_filename);
void debug_dump(const std::vector<note_t>& song, const char* const out_filename);

#endif /* UTILS_HH_ */
