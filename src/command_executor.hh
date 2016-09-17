#pragma once

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

void generate_bin_file(const std::string& lilypond_command,
                       const fs::path& input_lily_file,
                       const fs::path& output_bin_file,
                       const fs::path& output_tmp_directory,
		       std::ofstream& output_debug_file);
