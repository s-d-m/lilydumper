#include <stdexcept>
#include <fstream>
#include "utils.hh"


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


void debug_dump(const std::vector<note_t>& song, const char* const out_filename)
{
  std::ofstream file(out_filename,
		     std::ios::binary | std::ios::trunc | std::ios::out);

  if (not file.is_open())
  {
    return;
  }

  for (const auto& event : song)
  {
      file << event.start_time << "\n";
  }

  file.close();
}


void debug_dump(const std::vector<key_event>& song, const char* const out_filename)
{
  std::ofstream file(out_filename,
		     std::ios::binary | std::ios::trunc | std::ios::out);

  if (not file.is_open())
  {
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

  file.close();
}
