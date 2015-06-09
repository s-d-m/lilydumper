#ifndef STAFF_NUM_TO_INSTR_EXTRACTOR_HH
#define STAFF_NUM_TO_INSTR_EXTRACTOR_HH

#include <string>
#include <vector>

struct staff_to_instr_t
{
    uint8_t staff_number;
    std::string instr_name;
};

std::vector<staff_to_instr_t> get_staff_instr_mapping(const std::string& filename);

#endif
