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
#include "utils.hh" // for debug dump
#include "scope_exit.hh"

extern bool enable_debug_dump;
extern const char * debug_data_dir;

bool enable_debug_dump;
// it would be better not to use a raw pointer for debug_data_dir
// however since it is a global variable, it requires an exit-time destructor
// and a global constructor. so I can't use a std::string.
const char * debug_data_dir = nullptr;

struct options
{
    std::string notes_filename;
    std::string staff_num_to_instr_filename;
    std::vector<std::string> svg_files_with_skylines;
    std::vector<std::string> svg_files_without_skylines;
    std::string output_filename;
    bool enable_debug_dump = false;
    std::string debug_data_dir;
};

static
struct options get_options(const int argc, const char * const * argv)
{
  struct options res;

  for (unsigned int i = 1; i < static_cast<decltype(i)>(argc); ++i)
  {
    // create a string just to use operator==
    const auto str = std::string{argv[i]};

    if ((str == "-o") or (str == "--output-file"))
    {
      // next parameter will be the output file
      if (i == static_cast<decltype(i)>(argc) - 1)
      {
	// was the last parameter, so there is no filename behind it!
	throw std::runtime_error(std::string{"Error: '"} + str + "' must be followed by a filename");
      }

      if (not res.output_filename.empty())
      {
	throw std::runtime_error("Error, the output file must be specified only once.");
      }

      ++i;
      res.output_filename = argv[i];
    }
    else if ((str == "-n") or (str == "--notes-file"))
    {
      // next parameter will be the notes file
      if (i == static_cast<decltype(i)>(argc) - 1)
      {
	// was the last parameter, so there is no filename behind it!
	throw std::runtime_error(std::string{"Error: '"} + str + "' must be followed by a filename");
      }

      if (not res.notes_filename.empty())
      {
	throw std::runtime_error("Error, the notes file must be specified only once.");
      }

      ++i;
      res.notes_filename = argv[i];
    }
    else if ((str == "-s") or (str == "--staff-number-to-instrument-file"))
    {
      // next parameter will be the staff-number to instrument file
      if (i == static_cast<decltype(i)>(argc) - 1)
      {
	// was the last parameter, so there is no filename behind it!
	throw std::runtime_error(std::string{"Error: '"} + str + "' must be followed by a filename");
      }

      if (not res.staff_num_to_instr_filename.empty())
      {
	throw std::runtime_error("Error, the staff_num_to_instr_filename file must be specified only once.");
      }

      ++i;
      res.staff_num_to_instr_filename = argv[i];
    }
    else if (str == "--enable-debug")
    {
      res.enable_debug_dump = true;
    }
    else if (str == "--debug-dump-dir")
    {
      // next parameter will be the staff-number to instrument file
      if (i == static_cast<decltype(i)>(argc) - 1)
      {
	// was the last parameter, so there is no filename behind it!
	throw std::runtime_error(std::string{"Error: '"} + str + "' must be followed by a filename");
      }

      if (not res.debug_data_dir.empty())
      {
	throw std::runtime_error("Error, the directory du dump debug data must be specified only once.");
      }

      ++i;
      res.debug_data_dir = argv[i];
    }
    else
    {
      // it must be a svg file. is it one with or without skylines.
      // for now, keep it simple, request user to only pass svg with skylines,
      // and expect that for each svg file, there is one with skyline whose name
      // is the same one with ".no_skylines" at the end
      const auto filename = std::string{argv[i]};
      res.svg_files_with_skylines.push_back(filename);
      res.svg_files_without_skylines.push_back(filename + ".no_skylines");
    }
  }

  if (res.svg_files_without_skylines.empty())
  {
    throw std::runtime_error("Error, no svg files specified");
  }

  if (res.enable_debug_dump and res.debug_data_dir.empty())
  {
    throw std::runtime_error("Error, debug_dump requires you to specify a folder where to dump data");
  }

  return res;
}

static void usage(std::ostream& out, const char* const prog_name)
{
  out << "Usage: " << prog_name <<
    "[--enable-debug --debug-dump-dir <dirname>] "
    "--output-file <filename> "
    "--staff_num_to_instr_filename <filename> "
    "--notes_file <filename> "
    "<svg_file> [svg_files...]"
    "\n"
    "\n";
}

int main(int argc, const char * const * argv)
{
  try
  {
    if (argc == 1)
    {
      usage(std::cerr, argv[0]);
      return 1;
    }

    const auto options = get_options(argc, argv);
    enable_debug_dump = options.enable_debug_dump;
    SCOPE_EXIT( debug_data_dir = nullptr );
    debug_data_dir = options.debug_data_dir.c_str();

    const auto notes = get_notes(options.notes_filename);
    const auto staffs_to_instrument = get_staff_instr_mapping(options.staff_num_to_instr_filename);

    std::vector<svg_file_t> sheets;
    for (const auto& filename : options.svg_files_with_skylines)
    {
      sheets.emplace_back(get_svg_data(filename));
    }

    const auto keyboard_events = get_key_events(notes);
    debug_dump(keyboard_events, "key_events_final");
    const auto chords = get_chords(notes);
    const auto cursor_boxes = get_cursor_boxes(chords, sheets);
    const auto bar_num_events = get_bar_num_events(cursor_boxes);

    save_to_file(options.output_filename,
		 keyboard_events,
		 cursor_boxes,
		 bar_num_events,
		 staffs_to_instrument,
		 options.svg_files_without_skylines);
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << "\n";
    return 2;
  }
  return 0;
}
