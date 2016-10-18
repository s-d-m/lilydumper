#pragma once

#include <vector>
#include "utils.hh"

std::vector<note_t> get_unprocessed_notes(const fs::path& filename);
std::vector<note_t> get_processed_notes(const std::vector<note_t>& unprocessed_notes);
