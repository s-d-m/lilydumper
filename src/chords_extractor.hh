#pragma once

#include <vector>
#include "utils.hh"

// group notes into chords
std::vector<chord_t> get_chords(const std::vector<note_t>& notes);
