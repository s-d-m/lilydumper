#pragma once

#include <vector>
#include <string>

#include "keyboard_events_extractor.hh"
#include "cursor_boxes_extractor.hh"
#include "bar_number_events_extractor.hh"
#include "utils.hh"

void save_to_file(const fs::path& output_filename,
		  const std::vector<key_event>& keyboard_events,
		  const std::vector<cursor_box_t>& cursor_boxes,
		  const std::vector<bar_num_event_t>& bar_num_events,
		  const std::vector<std::string>& staff_num_mapping,
		  const std::vector<fs::path>& svg_filenames);
