#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>
#include "staff_num_to_instr_extractor.hh"


std::vector<staff_to_instr_t> get_staff_instr_mapping(const std::string& filename)
{
  std::ifstream file (filename, std::ios::in);
  if (! file.is_open() )
  {
    throw std::runtime_error(std::string{"Error: failed to open '"} + filename + "'");
  }

  std::vector<staff_to_instr_t> res;

  for (std::string line; std::getline(file, line); )
  {
    std::istringstream str (line);
    unsigned int instr_num;
    std::string instr_name;

    str >> instr_num
	>> instr_name;

    if (instr_num > std::numeric_limits<uint8_t>::max())
    {
      throw std::runtime_error("Error: staff number too big");
    }

    if (std::any_of(res.cbegin(), res.cend(), [=] (const auto& elt) {
	  return elt.staff_number == instr_num;
	}))
    {
      throw std::runtime_error("Error: the same staff number has been encountered twice");
    }

    if (instr_name == "")
    {
      std::cerr << "Warning: no instrument name found for staff "
		<< instr_num
		<< "\n";
    }

    res.emplace_back(staff_to_instr_t{
	.staff_number = static_cast<decltype(staff_to_instr_t::staff_number)>(instr_num),
	.instr_name = std::move(instr_name) }
      );
  }

  file.close();
  return res;
}
