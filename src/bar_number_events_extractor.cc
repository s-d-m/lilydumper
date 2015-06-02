#include "bar_number_events_extractor.hh"

std::vector<bar_num_event_t> get_bar_num_events(const std::vector<cursor_box_t>& cursor_boxes)
{
  std::vector<bar_num_event_t> res;
  if (cursor_boxes.empty())
  {
    return res;
  }

  // initialise last bar number with a value different than the first
  // bar number. It makes writing the loop a little easier. There is
  // no need to handle the special case of the first element. the if
  // will be taken on the first iteration through the loop
  auto last_bar_number = cursor_boxes[0].bar_number;
  last_bar_number++;

  for (const auto& cursor_box : cursor_boxes)
  {
    if (cursor_box.bar_number != last_bar_number)
    {
      last_bar_number = cursor_box.bar_number;
      res.emplace_back(bar_num_event_t{ .time = cursor_box.start_time,
					.bar_number = last_bar_number });
    }
  }

  return res;
}
