#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>
#include "staff_num_to_instr_extractor.hh"
#include "utils.hh" // for debug_dump

std::vector<std::string> get_staff_instr_mapping(const std::string& filename)
{
  // preconditions:
  // 1) all staff numbers are in the range [0 .. staff_num_mapping.size() - 1]
  // 2) all staff numbers are different
  // 3) they are sorted in ascending order
  //
  // as a consequence, one can store only the strings in a vector, and
  // the staff number is just the position in that vector

  std::ifstream file (filename, std::ios::in);
  if (! file.is_open() )
  {
    throw std::runtime_error(std::string{"Error: failed to open '"} + filename + "'");
  }

  std::vector<std::string> res;

  uint8_t current_staff_number = 0;
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

    if (instr_num != current_staff_number)
    {
      throw std::runtime_error(std::string{"Error: instrument numbers should start at 0 and be incremented."} +
			       "staff numbered " + std::to_string(instr_num) + " should actually be numbered " +
			       std::to_string(static_cast<long unsigned int>(current_staff_number)) + ".");
    }

    if (instr_name == "")
    {
      std::cerr << "Warning: no instrument name found for staff "
		<< instr_num
		<< "\n";
    }

    res.emplace_back( std::move(instr_name) );

    ++current_staff_number;
  }

  file.close();

  debug_dump(res, "instruments");
  return res;
}
