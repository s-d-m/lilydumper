#pragma once

#include <vector>
#include "cursor_boxes_extractor.hh"

struct bar_num_event_t
{
    decltype(cursor_box_t::start_time) time;
    decltype(cursor_box_t::bar_number) bar_number;
};

std::vector<bar_num_event_t> get_bar_num_events(const std::vector<cursor_box_t>& cursor_boxes);
