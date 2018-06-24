#include <iostream>
#include <vector>
#include <fstream>
#include "utils.hh"
#include "command_executor.hh"

extern const char * debug_data_dir;

// it would be better not to use a raw pointer for debug_data_dir
// however since it is a global variable, it requires an exit-time destructor
// and a global constructor. so I can't use a std::string.
const char * debug_data_dir = nullptr;

struct options
{
    options()
      : input_filename()
      , output_filename()
      , debug_data_dir()
      , lilypond_command()
    {
    }

    fs::path input_filename;
    fs::path output_filename;
    fs::path debug_data_dir;
    std::string lilypond_command;
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
    else if (str == "--debug-dump-dir")
    {
      // next parameter will be the directory name
      if (i == static_cast<decltype(i)>(argc) - 1)
      {
	// was the last parameter, so there is no directory behind it!
	throw std::runtime_error(std::string{"Error: '"} + str + "' must be followed by a directory name");
      }

      if (not res.debug_data_dir.empty())
      {
	throw std::runtime_error("Error, the debug-dump directory must be specified only once.");
      }

      ++i;
      res.debug_data_dir = argv[i];
    }
    else if ((str == "-i") or (str == "--input-file"))
    {
      // next parameter will be the input file name
      if (i == static_cast<decltype(i)>(argc) - 1)
      {
	// was the last parameter, so there is input file behind it!
	throw std::runtime_error(std::string{"Error: '"} + str + "' must be followed by a filename");
      }

      if (not res.input_filename.empty())
      {
	throw std::runtime_error("Error, the input filename must be specified only once.");
      }

      ++i;
      res.input_filename = argv[i];
    }
    else if ((str == "-c") or (str == "--lilypond-command"))
    {
      // next parameter will be the command name
      if (i == static_cast<decltype(i)>(argc) - 1)
      {
	// was the last parameter, so there is no command behind it!
	throw std::runtime_error(std::string{"Error: '"} + str + "' must be followed by a command");
      }

      if (not res.lilypond_command.empty())
      {
	throw std::runtime_error("Error, the lilypond command must be specified only once.");
      }

      ++i;
      res.lilypond_command = argv[i];
    }
    else
    {
      throw std::runtime_error(std::string{"Error, unknown option '"} + str + "'.");
    }
  }

  // finished parsing options

  // ensures all mandatory fields have been set
  if (res.input_filename.empty())
  {
    throw std::runtime_error(std::string{"Error, missing input file"});
  }

  // set defaults values for optional and unset values
  if (res.lilypond_command.empty())
  {
    res.lilypond_command = "lilypond";
  }

  if (res.debug_data_dir.empty())
  {
    res.debug_data_dir = get_temp_dir().string();
    std::cout << "Using directory '" << res.debug_data_dir << "'.\n";
  }

  if (res.output_filename.empty())
  {
    res.output_filename = fs::path{res.debug_data_dir} / res.input_filename.filename().replace_extension("bin");
  }

  return res;
}

static void usage(std::ostream& out, const char* const prog_name)
{
  out << "Usage: " << prog_name <<
    "[--debug-dump-dir <dirname>] "
    "[-o|--output-file <filename>] "
    "[-c|--lilypond-command <filename>] "
    "-i|--input-file <filename>"
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
    debug_data_dir = options.debug_data_dir.c_str();
    const auto debug_output_file = options.debug_data_dir / "logs";
    std::ofstream log_stream (debug_output_file.string());

    try
    {
      log_stream << "Program was launched with the following command:\n ";
      for (unsigned int i = 0; i < static_cast<unsigned>(argc); ++i)
      {
	log_stream << " '" << argv[i] << "'";
      }
      log_stream << "\n\n";

      generate_bin_file(options.lilypond_command, options.input_filename, options.output_filename,
			options.debug_data_dir, log_stream);
    }
    catch (const std::exception& e)
    {
      log_stream << e.what() << "\n";
      throw;
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << "\n";
    return 2;
  }
  return 0;
}
