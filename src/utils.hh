#ifndef UTILS_HH_
#define UTILS_HH_

#include <experimental/filesystem>
#include <string>
#include <vector>
#include "notes_file_extractor.hh" // for note_t definition
#include "keyboard_events_extractor.hh" // for key_event definition
#include "chords_extractor.hh" // for chord_t definition

std::string get_value_from_field(const std::string& id_str, const char* const field);

void debug_dump(const std::vector<key_event>& song, const char* const out_filename);
void debug_dump(const std::vector<note_t>& song, const char* const out_filename);
void debug_dump(const std::vector<chord_t>& chord, const char* const out_filename);
void debug_dump(const std::vector<std::string>& strings, const char* const out_filename);

namespace fs = std::experimental::filesystem;

fs::path get_temp_dir();

#endif /* UTILS_HH_ */
