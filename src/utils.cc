#include <stdlib.h>
#include <cstring>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include "utils.hh"

extern bool enable_debug_dump;
extern const char * debug_data_dir;


// this function takes an id string, and return the value for the requested field
// throw in the field could not be found.
// an id string is a string made of field=value separated by the '#' symbol.
// hence field and values can't contain '#' or '='.
// important to note is that the string must starts and end by a '#'. This makes
// it easier to look into it, as there is no corner case.
// an example of an id field is: "#x-width=1.389984#y-height=1.100012#origin=././foo.ly:17:25:27#pitch=83#has-tie-attached=no#staff-number=0#duration-string=2#duration=50000000#is-grace-note=no#"
std::string get_value_from_field(const std::string& id_str, const char* const field)
{
  const auto field_pos = id_str.find(field);
  if (field_pos == std::string::npos)
  {
    throw std::runtime_error(std::string{"Error: couldn't find field "} + field + " in string [" + id_str + "]");
  }

  const auto eq_pos = id_str.find("=", field_pos);
  if (eq_pos == std::string::npos)
  {
    throw std::runtime_error(std::string{"Error: invalid is string. Field "} + field + " is not followed by '=' character");
  }

  const auto hash_pos = id_str.find("#", eq_pos);
  if (hash_pos == std::string::npos)
  {
    throw std::runtime_error(std::string{"Error: invalid is string. Field "} + field + " is not followed by '#' character");
  }

  const auto start_value = eq_pos + 1;
  return id_str.substr(start_value, hash_pos - start_value);
}

static
fs::path get_debug_filename_full_path(const char* const out_filename)
{
  if (debug_data_dir == nullptr)
  {
    throw std::runtime_error("Error: directory to output debug data is unset (nullptr)");
  }

  return fs::path(std::string{debug_data_dir} + "/" + out_filename);
}

void debug_dump(const std::vector<note_t>& song, const char* const out_filename)
{
  const auto out_file = get_debug_filename_full_path(out_filename);

  std::ofstream file(out_file,
		     std::ios::binary | std::ios::trunc | std::ios::out);

  if (not file.is_open())
  {
    std::cerr << "Error: could not open " << out_file << " for writing.\n";
    return;
  }

  for (const auto& event : song)
  {
      file << event.start_time << "\n";
  }

}


void debug_dump(const std::vector<key_event>& song, const char* const out_filename)
{
  const auto out_file = get_debug_filename_full_path(out_filename);

  std::ofstream file(out_file,
		     std::ios::binary | std::ios::trunc | std::ios::out);

  if (not file.is_open())
  {
    std::cerr << "Error: could not open " << out_file << " for writing.\n";
    return;
  }

  for (const auto& event : song)
  {
    if (event.data.ev_type == key_data::pressed)
    {
      file << event.time << " down " << static_cast<int>(event.data.pitch) << "\n";
    }

    if (event.data.ev_type == key_data::released)
    {
      file << event.time << " up " << static_cast<int>(event.data.pitch) << "\n";
    }

  }

}


void debug_dump(const std::vector<chord_t>& chords, const char* const out_filename)
{
  const auto out_file = get_debug_filename_full_path(out_filename);

  std::ofstream file(out_file,
		     std::ios::binary | std::ios::trunc | std::ios::out);

  if (not file.is_open())
  {
    std::cerr << "Error: could not open " << out_file << " for writing.\n";
    return;
  }

  for (const auto& chord : chords)
  {
    const auto start_time = chord.notes[0].start_time;
    file << start_time;

    for (const auto& note : chord.notes)
    {
      file << "\n  " << static_cast<int>(note.pitch) << " -> " << note.stop_time;
    }

    file << "\n\n";
  }
}


void debug_dump(const std::vector<std::string>& strings, const char* const out_filename)
{
  const auto out_file = get_debug_filename_full_path(out_filename);

  std::ofstream file(out_file,
		     std::ios::binary | std::ios::trunc | std::ios::out);

  if (not file.is_open())
  {
    std::cerr << "Error: could not open " << out_file << " for writing.\n";
    return;
  }

  for (const auto& string : strings)
  {
    file << string << "\n";
  }

}

fs::path get_temp_dir()
{
  const auto sys_temp_dir = fs::temp_directory_path() /= "lilydumper_XXXXXX";

  // TODO: rework this when compilers will support C++17. std::string::data() returns a pointer to non-const data
  // in C++17, so one can just pass path_to_dir.data() to mkdtemp instead of creating a useless copy.

  std::string path_to_dir (sys_temp_dir.string());

  // const auto res_str = mkdtemp(path_to_dir.data());
  // if (res == nullptr)
  // {
  //   throw std::runtime_error("Unable to create a temporary directory.");
  // }

  // return fs::path(res_str);

  struct dummy_writeable_string
  {
      struct Deleter
      {
	  void operator()(char* data) const
	  {
	    delete[] data;
	  }
      };

      explicit dummy_writeable_string(const std::string& str)
	: _data (new char[str.size() + 1])
      {
	std::memcpy(_data.get(), str.data(), str.size());
	_data[str.size()] = '\0';
      }

      char* data()
      {
	return _data.get();
      }

    private:
      std::unique_ptr<char[], Deleter> _data;
  };

  dummy_writeable_string dummy (path_to_dir);
  const auto res_str = mkdtemp(dummy.data());
  if (res_str == nullptr)
  {
    throw std::runtime_error("Unable to create a temporary directory.");
  }

  return fs::path(res_str);
}
