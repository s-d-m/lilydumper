#include <iostream>
#include <vector>

#include "svg_extractor.hh"
#include "notes_file_extractor.hh"
#include "chords_extractor.hh"

int main(int argc, const char * const * argv)
{
  if (argc == 1)
  {
    std::cerr << "Usage: " << argv[0] << " xml_file [xml_files... ] note_file\n";
    return 1;
  }

  std::vector<note_t> notes;
  std::vector<svg_file_t> sheets;

  for (unsigned int i = 1; i < static_cast<decltype(i)>(argc); ++i)
  {
    const auto filename = std::string{argv[i]};

    if (filename.find(".notes") != std::string::npos)
    {
      // notes file
      notes = get_notes(filename);
    }
    else
    {
      // svg file
      sheets.emplace_back(get_svg_data(filename));
    }
  }

  const auto chords = get_chords(notes);
  return 0;
}
