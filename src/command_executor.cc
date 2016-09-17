#include <algorithm>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include "command_executor.hh"
#include "event_listener.h"
#include "open_preloader.h"
#include "common.h"

#include "svg_extractor.hh"
#include "notes_file_extractor.hh"
#include "chords_extractor.hh"
#include "cursor_boxes_extractor.hh"
#include "keyboard_events_extractor.hh"
#include "bar_number_events_extractor.hh"
#include "staff_num_to_instr_extractor.hh"
#include "file_exporter.hh"
#include "utils.hh"

constexpr const char* const without_skyline_suffix = ".without_skylines";

static
void copy_buffer_to(const char* buffer, int buf_len, const fs::path& dst_file)
{
  std::ofstream file (dst_file.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
  if (file.fail())
  {
    throw std::runtime_error(std::string{"Failed to open ["} + dst_file.c_str() + "] for writing\n");
  }

  file.write(buffer, buf_len);
}

static
void copy_event_listener_to(const fs::path& dst_file)
{
  copy_buffer_to(reinterpret_cast<const char*>(event_listener_scm), event_listener_scm_len, dst_file);
}

static
void copy_open_preloader_to(const fs::path& dst_file)
{
  copy_buffer_to(reinterpret_cast<const char*>(open_preloader_so), open_preloader_so_len, dst_file);
}

static
bool execute_command(const std::vector<std::string>& command,
		     const std::vector<std::string>& env,
		     std::ofstream& output_debug_file)
{
  const auto nb_param = command.size();

  if (nb_param == 0)
  {
    throw std::runtime_error("Error: can't execute an empty command");
  }

  auto pid = fork();
  if (pid == -1)
  {
    throw std::runtime_error(std::string{"Couldn't launch the ["} + command[0] + "] command");
  }

  if (pid == 0)
  {

    // TODO: rework this when compilers will support C++17. std::string::data() returns a pointer to non-const data
    // in C++17, so one can just pass command[i].data() to execvp instead of creating useless copy

    struct dummy_to_avoid_vla
    {
	struct Deleter
	{
	    void operator()(char* data[]) const
	    {
	      delete[] data;
	    }
	};

	dummy_to_avoid_vla(const std::vector<std::string>& _command)
	  : _data(new char*[_command.size() + 1])
	{
	  const auto len = _command.size();
	  for (unsigned int i = 0; i < len; ++i)
	  {
	    _data[i] = const_cast<char*>(_command[i].data()); // TODO: const cast shouldn't be required with C++17
	  }
	  _data[len] = nullptr;
	}

	char** data()
	{
	  return _data.get();
	}

    private:
	std::unique_ptr<char*[], Deleter> _data;
    };

    // child
    dummy_to_avoid_vla c_command (command);
    dummy_to_avoid_vla c_env (env);

    ::execvpe(c_command.data()[0], c_command.data(), c_env.data());
    throw std::runtime_error(std::string{"Failed to execute ["} + command[0] + "] command (" + strerror(errno) + ")");
  }

  int status;
  waitpid(pid, &status, 0);

  const auto print_command = [] (std::ofstream& stream ,const auto& _command) {
    for (const auto& str : _command)
    {
      stream << " " << str;
    }
  };

  if (not WIFEXITED(status))
  {
    output_debug_file << "Failed to execute command [";
    print_command(output_debug_file, command);
    output_debug_file << "]\n";
    return false;
  }

  const auto exit_code = WEXITSTATUS(status);
  if (exit_code != 0)
  {
    output_debug_file << "command [";
    print_command(output_debug_file, command);
    output_debug_file << "]\n   exited with error code " << exit_code << "\n";
    return false;
  }

  output_debug_file << "command [";
  print_command(output_debug_file, command);
  output_debug_file << "] successed\n";
  return true;
}


static
bool execute_command_with_append_to_env(const std::vector<std::string>& command,
					const std::vector<std::string>& to_append,
					std::ofstream& output_debug_file)
{
  std::vector<std::string> env;

  for (unsigned int i = 0; environ[i] != nullptr; ++i)
  {
    env.push_back(environ[i]);
  }

  for (const auto& str : to_append)
  {
    env.push_back(str);
  }

  return execute_command(command, env, output_debug_file);
}

static
bool execute_command(const std::vector<std::string>& command,
		     std::ofstream& output_debug_file)
{
  return execute_command_with_append_to_env(command, std::vector<std::string>{}, output_debug_file);
}

static
std::tuple<fs::path, fs::path> generate_note_and_staff_num_files(const std::string& lilypond_command,
								 const fs::path& input_lily_file,
								 const fs::path& output_tmp_directory,
								 std::ofstream& output_debug_file)
{
  // must run lilypond with force unfold repeat
  const std::string event_listener_filename = "event-listener.scm";
  const fs::path out_listener_file = output_tmp_directory / event_listener_filename;
  const fs::path out_preloader_file = output_tmp_directory / "open_preloader.so";
  const fs::path out_patched_file = output_tmp_directory / PATCHED_FILE_NAME;

  const fs::path lily_filename = input_lily_file.filename();
  const auto out_lily_with_ext = [&] (const char* extension) {
    auto res = output_tmp_directory / lily_filename;
    res.replace_extension(extension);
    return res;
  };

  const fs::path out_note_file = out_lily_with_ext(".notes");
  const fs::path out_staff_num_file = out_lily_with_ext(".sn2in");

  copy_event_listener_to(out_listener_file);
  copy_open_preloader_to(out_preloader_file);

  const std::vector<std::string> command_line {
    { lilypond_command,
	"-dno-point-and-click",
	std::string{"--output="} + output_tmp_directory.c_str(),
	"--evaluate=(ly:add-option 'note-file-output #f  \"Output for the note file. Default is filename with .notes extension instead of .ly\")",
	std::string{"--evaluate=(ly:set-option 'note-file-output \""} + out_note_file.c_str() + "\")",
	"--evaluate=(ly:add-option 'instrument-name-file-output #f  \"Output for the staff-number-to-instrument-name-table file. Default is filename with .sn2in extension instead of .ly\")",
	std::string{"--evaluate=(ly:set-option 'instrument-name-file-output \""} + out_staff_num_file.c_str() + "\")",
	std::string{"-dinclude-settings=\""} + out_listener_file.c_str() + "\"",
	"-dbackend=null",
	input_lily_file.c_str() } };


  const std::vector<std::string> env { { std::string{"LD_PRELOAD="} + out_preloader_file.c_str() },
					 std::string{DUMP_OUTPUT_DIR} + "=" + output_tmp_directory.c_str() };
  const auto ret = execute_command_with_append_to_env(command_line, env, output_debug_file);

  if (not ret)
  {
    throw std::runtime_error("Failed to create the notes and staff-num-to-instrument name files");
  }

  const auto is_file_ok = [&] (const auto& file) {
    if ((not fs::exists(file)) or (not is_regular_file(file)))
    {
      output_debug_file << "  Failed to create [" << file.c_str() << "]\n";
      return false;
    }
    else
    {
      output_debug_file << "  Detected expected output file [" << file.c_str() << "]\n";
      return true;
    }
  };

  const bool has_error = [&](){
    const auto note_ok = is_file_ok(out_note_file);
    const auto staff_ok = is_file_ok(out_staff_num_file);
    const auto patched_ok = is_file_ok(out_patched_file);
    return not (note_ok and staff_ok and patched_ok);
  }();

  output_debug_file << "\n";

  if (has_error)
  {
    throw std::runtime_error("Failed to create the notes and staff-num-to-instrument name files");
  }


  return std::make_tuple(out_note_file, out_staff_num_file);
}

static
std::vector<fs::path> generate_svg_files_without_skylines(const std::string& lilypond_command,
							  const fs::path& input_lily_file,
							  const fs::path& output_tmp_directory,
							  std::ofstream& output_debug_file)
{
  const std::vector<std::string> command_line {
    { lilypond_command,
	"-dno-point-and-click",
	std::string{"--output="} + output_tmp_directory.c_str(),
	"-dbackend=svg",
	input_lily_file.c_str() } };

  const auto ret = execute_command(command_line, output_debug_file);
  if (not ret)
  {
    throw std::runtime_error("Failed to create the SVGs files (without skylines)");
  }


  std::vector<fs::path> svg_files;
  for (const auto& file : fs::directory_iterator(output_tmp_directory))
  {
    const auto& path = file.path();

    if (fs::is_regular_file(path) and (path.extension() == ".svg"))
    {
      svg_files.push_back(file);
    }
  }

  const auto nb_svgs = svg_files.size();
  if (nb_svgs == 0)
  {
    throw std::runtime_error("Error: no SVGs files (the ones without skylines) were created in the temporary directory");
  }

  std::sort(std::begin(svg_files), std::end(svg_files), [] (const auto& a, const auto& b) {
      return fs::last_write_time(a) < fs::last_write_time(b);
    });

  output_debug_file << "Found " << nb_svgs << " svgs files without skylines:\n";

  for (auto& svg_file : svg_files)
  {
    const auto new_name = [] (const auto& file_name) {
      auto res = file_name;
      res += without_skyline_suffix;
      return res;
    }(svg_file);

    fs::rename(svg_file, new_name);
    output_debug_file << "  " << new_name << "\n";
    svg_file = new_name;
  }

  output_debug_file << "\n";

  return svg_files;
}

static
std::vector<fs::path> generate_svg_files_with_skylines(const std::string& lilypond_command,
						       const fs::path& input_lily_file,
						       const fs::path& output_tmp_directory,
						       std::ofstream& output_debug_file)
{

  const std::string event_listener_filename = "event-listener.scm";
  const auto dst_event_listener_file = output_tmp_directory / event_listener_filename;

  copy_event_listener_to(dst_event_listener_file);

  const std::vector<std::string> command_line {
    { lilypond_command,
	"-dno-point-and-click",
	std::string{"--output="} + output_tmp_directory.c_str(),
	std::string{"-dinclude-settings="} + dst_event_listener_file.c_str(),
	"-dbackend=svg",
	input_lily_file.c_str() } };

  const auto ret = execute_command(command_line, output_debug_file);
  if (not ret)
  {
    throw std::runtime_error("Failed to create the SVGs files (without skylines)");
  }


  std::vector<fs::path> svg_files;
  for (const auto& file : fs::directory_iterator(output_tmp_directory))
  {
    const auto& path = file.path();

    if (fs::is_regular_file(path) and (path.extension() == ".svg"))
    {
      svg_files.push_back(file);
    }
  }

  const auto nb_svgs = svg_files.size();
  if (nb_svgs == 0)
  {
    throw std::runtime_error("Error: no SVGs files (the ones with skylines) were created in the temporary directory");
  }

  std::sort(std::begin(svg_files), std::end(svg_files), [] (const auto& a, const auto& b) {
      return fs::last_write_time(a) < fs::last_write_time(b);
    });

  output_debug_file << "Found " << nb_svgs << " svgs files with skylines:\n";

  for (const auto& svg_file : svg_files)
  {
    output_debug_file << "  " << svg_file << "\n";
  }
  output_debug_file << "\n";

  return svg_files;
}


void generate_bin_file(const std::string& lilypond_command,
		       const fs::path& input_lily_file,
		       const fs::path& output_bin_file,
		       const fs::path& output_tmp_directory,
		       std::ofstream& output_debug_file)
{
  const auto [notes_file, staffs_num_file] = generate_note_and_staff_num_files(lilypond_command, input_lily_file,
									       output_tmp_directory, output_debug_file);

  const auto svgs_without_skylines = generate_svg_files_without_skylines(lilypond_command,
									 input_lily_file,
									 output_tmp_directory,
									 output_debug_file);
  const auto svgs_with_skylines = generate_svg_files_with_skylines(lilypond_command,
								   input_lily_file,
								   output_tmp_directory,
								   output_debug_file);

  // safety check: there should be the same number of images with and without skylines
  const auto nb_svgs = svgs_with_skylines.size();
  const auto nb_svgs_without_skylines = svgs_without_skylines.size();
  if (nb_svgs != nb_svgs_without_skylines)
  {
    throw std::runtime_error(std::string{"Number of svg files with skylines and without mismatch.\n"
	  "  There are "} + std::to_string(nb_svgs) + " svgs with skylines but " +
      std::to_string(nb_svgs_without_skylines) + "without.\n");
  }

  // safety check: they should have the same names (except the without skyline suffix)
  for (unsigned int i = 0; i < nb_svgs; ++i)
  {
    const auto name_without = svgs_without_skylines[i].string();
    const auto name_with = svgs_with_skylines[i].string();
    if (name_without != (name_with + without_skyline_suffix))
    {
      throw std::runtime_error(std::string{"SVG filename mismatch detected.\n"
	    "  One file is named ["} + name_without +"]\n  and the associated one with skyline is ["
	+ name_with + "]\n  The expected names are [" + name_without+ "] and ["
	+ name_with + without_skyline_suffix + "]\n");
    }
  }


  const auto notes = get_notes(notes_file);
  const auto staffs_to_instrument = get_staff_instr_mapping(staffs_num_file);

  std::vector<svg_file_t> sheets;
  for (const auto& filename : svgs_with_skylines)
  {
    sheets.emplace_back(get_svg_data(filename));
  }

  const auto keyboard_events = get_key_events(notes);
  const auto chords = get_chords(notes);
  const auto cursor_boxes = get_cursor_boxes(chords, sheets);
  const auto bar_num_events = get_bar_num_events(cursor_boxes);

  save_to_file(output_bin_file,
	       keyboard_events,
	       cursor_boxes,
	       bar_num_events,
	       staffs_to_instrument,
	       svgs_without_skylines);
}
