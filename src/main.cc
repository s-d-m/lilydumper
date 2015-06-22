#include <iostream>
#include <vector>

#include "svg_extractor.hh"
#include "notes_file_extractor.hh"
#include "chords_extractor.hh"
#include "cursor_boxes_extractor.hh"
#include "keyboard_events_extractor.hh"
#include "bar_number_events_extractor.hh"
#include "staff_num_to_instr_extractor.hh"
#include "file_exporter.hh"

int main(int argc, const char * const * argv)
{
  if (argc == 1)
  {
    std::cerr << "Usage: " << argv[0] << " xml_file [xml_files... ] note_file\n";
    return 1;
  }

  std::vector<note_t> notes;
  std::vector<svg_file_t> sheets;
  std::vector<staff_to_instr_t> staffs_to_instrument;
  std::vector<std::string> svg_filenames;
  std::string output_filename;

  for (unsigned int i = 1; i < static_cast<decltype(i)>(argc); ++i)
  {
    const auto filename = std::string{argv[i]};

    if (filename.find(".notes") != std::string::npos)
    {
      // notes file
      if (not notes.empty())
      {
	throw std::runtime_error("Error: only one note file can be processed per music sheet");
      }
      notes = get_notes(filename);
    }
    else if (filename.find(".sn2in") != std::string::npos)
    {
      // staff number to instrument mapping
      if (not staffs_to_instrument.empty())
      {
	throw std::runtime_error("Error: only one sn2in file can be processed per music sheet");
      }

      staffs_to_instrument = get_staff_instr_mapping(filename);
    }
    else if (filename == "-o")
    {
      // not a filename, the real one will be the next parameter
      if (i == static_cast<decltype(i)>(argc) - 1)
      {
	// was the last parameter, so there is no filename behind it!
	throw std::runtime_error("Error: -o must be followed by a filename");
      }

      ++i;
      output_filename = argv[i];
    }
    else
    {
      // svg file
      svg_filenames.emplace_back(filename + ".no_skylines");
      sheets.emplace_back(get_svg_data(filename));
    }
  }

  const auto keyboard_events = get_key_events(notes);
  const auto chords = get_chords(notes);
  const auto cursor_boxes = get_cursor_boxes(chords, sheets);
  const auto bar_num_events = get_bar_num_events(cursor_boxes);

  save_to_file(output_filename,
	       keyboard_events,
	       cursor_boxes,
	       bar_num_events,
	       staffs_to_instrument,
	       svg_filenames);

  return 0;
}
