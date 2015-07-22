#ifndef STAFF_NUM_TO_INSTR_EXTRACTOR_HH
#define STAFF_NUM_TO_INSTR_EXTRACTOR_HH

#include <string>
#include <vector>

// staff number must be 0 .. nb_staff - 1
// therefore the staff number -> name association will be as simple
// as the position of the string in the vector
std::vector<std::string> get_staff_instr_mapping(const std::string& filename);

#endif