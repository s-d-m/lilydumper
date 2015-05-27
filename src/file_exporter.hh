#ifndef FILE_EXPORTER_HH_
#define FILE_EXPORTER_HH_

#include <vector>
#include <string>

#include "keyboard_events_extractor.hh"
#include "cursor_boxes_extractor.hh"

void save_events_to_file(const std::string& output_filename,
			 const std::vector<key_event>& keyboard_events,
			 const std::vector<cursor_box_t>& cursor_boxes);

#endif /* FILE_EXPORTER_HH_ */
