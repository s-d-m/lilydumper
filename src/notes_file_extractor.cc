#include <string>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "notes_file_extractor.hh"
#include "utils.hh"

std::vector<note_t> get_notes(const std::string& filename)
{
  std::ifstream file (filename, std::ios::in);
  if (! file.is_open() )
  {
    throw std::runtime_error(std::string{"Error: failed to open '"} + filename + "'");
  }

  std::vector<note_t> res;

  for (std::string line; std::getline(file, line); )
  {
    std::istringstream str (line);
    std::string line_type;
    std::string field1;
    uint64_t start_time;
    std::string field2;
    uint64_t stop_time;
    std::string field3;
    unsigned int bar_number;
    std::string field4;
    std::string id_str;

    str >> line_type
	>> field1
	>> start_time
	>> field2
	>> stop_time
	>> field3
	>> bar_number
	>> field4
	>> id_str;

    if (line_type != "note")
    {
      throw std::runtime_error(std::string{"Error: invalid line found in '"}
			       + filename + "'. Starts by '" + field1
			       + "' instead of either 'note'");
    }

    if (field1 != "start-time:")
    {
      throw std::runtime_error(std::string{"Error: invalid fieldname found in '"}
			       + filename + "'. found '" + field1
			       + "' instead of either 'start-time:'");
    }

    if (field2 != "stop-time:")
    {
      throw std::runtime_error(std::string{"Error: invalid fieldname found in '"}
			       + filename + "'. found '" + field2
			       + "' instead of 'stop-time:'");

    }

    if (field3 != "bar-number:")
    {
      throw std::runtime_error(std::string{"Error: invalid fieldname found in '"}
			       + filename + "'. found '" + field3
			       + "' instead of 'bar-number:'");

    }

    if (field4 != "id:")
    {
      throw std::runtime_error(std::string{"Error: invalid fieldname found in '"}
			       + filename + "'. found '" + field4
			       + "' instead of 'id:'");

    }

    res.emplace_back(note_t{
	.start_time = start_time,
	.stop_time = stop_time,
        .bar_number = static_cast<decltype(note_t::bar_number)>(bar_number),
	.pitch = static_cast<decltype(note_t::pitch)>(std::stoul(get_value_from_field(id_str, "pitch"))),
	.is_played = true, // set to true for now. second pass will set this value based on ties
	.id = std::move(id_str) }
      );
  }

  // res is not sorted now due to the current handling of grace notes.
  std::stable_sort(std::begin(res), std::end(res), [] (const auto& a, const auto& b) {
      return a.start_time < b.start_time;
    });

  return res;
}
