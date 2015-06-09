#ifndef FILE_EXPORTER_HH_
#define FILE_EXPORTER_HH_

#include <vector>
#include <string>

#include "keyboard_events_extractor.hh"
#include "cursor_boxes_extractor.hh"
#include "bar_number_events_extractor.hh"
#include "staff_num_to_instr_extractor.hh"

void save_events_to_file(const std::string& output_filename,
			 const std::vector<key_event>& keyboard_events,
			 const std::vector<cursor_box_t>& cursor_boxes,
			 const std::vector<bar_num_event_t>& bar_num_events,
			 const std::vector<staff_to_instr_t>& staff_num_mapping);

#endif /* FILE_EXPORTER_HH_ */
