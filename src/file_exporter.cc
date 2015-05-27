#include <fstream>
#include <stdexcept>
#include "file_exporter.hh"

enum event_type : uint8_t
{
  press_key      = 0,
  release_key    = 1,
  set_bar_number = 2,
  set_cursor     = 3,
  set_svg_file   = 4,
};

static
uint8_t get_nb_events_at_time(const std::vector<key_event>::const_iterator& key_event_it,
			      const std::vector<cursor_box_t>::const_iterator& cursor_event_it,
			      const std::vector<key_event>::const_iterator& end_key,
			      const std::vector<cursor_box_t>::const_iterator& end_cursor,
			      const uint64_t current_timing_event)
{
  unsigned int nb_key_events = 0;
  static_assert(std::numeric_limits<decltype(nb_key_events)>::max() >= std::numeric_limits<uint8_t>::max(),
		"nb_events can constain key events and cursor events. Therefore the maximum number of key_events must be at least as big as the number of all events so avoid wrapping when uncessary");

  for (auto it = key_event_it;
       (it != end_key) and (it->time == current_timing_event);
       ++it)
  {
    if (nb_key_events == std::numeric_limits<uint8_t>::max())
    {
      throw std::logic_error(std::string{"Error: can't handle more than "} +
			     std::to_string(std::numeric_limits<uint8_t>::max()) +
			     "events occuring at the same time");
    }

    ++nb_key_events;
  }

  unsigned int nb_cursor_events = 0;
  static_assert(std::numeric_limits<decltype(nb_cursor_events)>::max() >= std::numeric_limits<uint8_t>::max(),
		"nb_events can constain key events and cursor events. Therefore the maximum number of cursor_events must be at least as big as the number of all events so avoid wrapping when uncessary");

  for (auto it = cursor_event_it;
       (it != end_cursor) and (it->start_time == current_timing_event);
       ++it)
  {
    ++nb_cursor_events;

    // sanity check
    if (nb_cursor_events > 1)
    {
      throw std::logic_error("Error: can't move the cursor twice (or more) at the same.");
    }
  }

  // sanity check
  if ((nb_cursor_events + nb_key_events) > std::numeric_limits<uint8_t>::max())
  {
    throw std::logic_error(std::string{"Error: can't handle more than "} +
			   std::to_string(std::numeric_limits<uint8_t>::max()) +
			   "events occuring at the same time");
  }

  return static_cast<uint8_t>(nb_cursor_events + nb_key_events);
}



template <typename T>
void output_as_big_endian(std::ofstream& out, const T value)
{
  // output timing such that number is saved as big endian
  for (unsigned int i = 0; i < sizeof(T); ++i)
  {
    out << static_cast<uint8_t>((value >> (8 * (sizeof(T) - i - 1))) & 0xFF);
  }
}

static
decltype(key_event::time) get_current_event_timing(const std::vector<key_event>::const_iterator& key_event_it,
						   const std::vector<cursor_box_t>::const_iterator& cursor_event_it,
						   const std::vector<key_event>::const_iterator& end_key,
						   const std::vector<cursor_box_t>::const_iterator& end_cursor)
{
  constexpr auto invalid_value = std::numeric_limits<decltype(key_event::time)>::max();
  auto res = invalid_value;
  if (key_event_it != end_key)
  {
    res = std::min(res, key_event_it->time);
  }

  if (cursor_event_it != end_cursor)
  {
    res = std::min(res, cursor_event_it->start_time);
  }

  // sanity check
  if (res == invalid_value)
  {
    throw std::runtime_error("Error: invalid timing event");
  }

  static_assert(sizeof(key_event::time) == sizeof(cursor_box_t::start_time),
		"events are going to be grouped by occuring time, hence time of occurence must be comparable with no possible data loss");


  return res;
}

static
void output_cursor_move_event(std::ofstream& out,
			      const cursor_box_t& cursor,
			      decltype(cursor_box_t::svg_file_pos)& current_svg_file)
{

  // does the new cursor appear to be on a different page?
  const auto this_svg_pos = cursor.svg_file_pos;
  if (this_svg_pos != current_svg_file)
  {
    // move on to a new page
    static_assert(sizeof(event_type::set_svg_file) == 1, "an event type should be one byte");
    output_as_big_endian(out, event_type::set_svg_file);

    static_assert(sizeof(this_svg_pos) == 2, "svg file pos should be 2bytes");
    output_as_big_endian(out, this_svg_pos);

    // report to caller the new page number we are on.
    current_svg_file = this_svg_pos;
  }

  // output the data for the cursor move
  static_assert(sizeof(event_type::set_cursor) == 1, "an event type should be one byte");
  output_as_big_endian(out, event_type::set_cursor);

  static_assert(sizeof(cursor.left) == 4, "each cursor coord must be 4 bytes");
  static_assert(sizeof(cursor.right) == 4, "each cursor coord must be 4 bytes");
  static_assert(sizeof(cursor.top) == 4, "each cursor coord must be 4 bytes");
  static_assert(sizeof(cursor.bottom) == 4, "each cursor coord must be 4 bytes");

  output_as_big_endian(out, cursor.left);
  output_as_big_endian(out, cursor.right);
  output_as_big_endian(out, cursor.top);
  output_as_big_endian(out, cursor.bottom);

}

static
void output_key_event(std::ofstream& out, const key_data& key)
{
  if ((key.ev_type != key_data::type::pressed) and
      (key.ev_type != key_data::type::released))
  {
    throw std::runtime_error("key event type could only be pressed or released");
  }

  if (key.ev_type == key_data::type::pressed)
  {
    static_assert(sizeof(event_type::press_key) == 1, "an event type should be one byte");
    output_as_big_endian(out, event_type::press_key);

    static_assert(sizeof(key.pitch) == 1, "pitch should be one byte");
    output_as_big_endian(out, key.pitch);

    static_assert(sizeof(key.staff_number) == 1, "staff number should be one byte");
    output_as_big_endian(out, key.staff_number);
  }
  else
  {
    static_assert(sizeof(event_type::release_key) == 1, "an event type should be one byte");
    output_as_big_endian(out, event_type::release_key);

    static_assert(sizeof(key.pitch) == 1, "pitch should be one byte");
    output_as_big_endian(out, key.pitch);
  }
}



static
void output_events_data(std::ofstream& out,
			const std::vector<key_event>& keyboard_events,
			const std::vector<cursor_box_t>& cursor_boxes)
{
  auto key_event_it = keyboard_events.cbegin();
  auto cursor_event_it = cursor_boxes.cbegin();

  const auto end_key = keyboard_events.cend();
  const auto end_cursor = cursor_boxes.cend();

  auto current_svg_file = std::numeric_limits<decltype(cursor_box_t::svg_file_pos)>::max();

  while ((key_event_it != end_key) and (cursor_event_it != end_cursor))
  {
    // output timing
    const auto current_timing = get_current_event_timing(key_event_it, cursor_event_it,
							  end_key, end_cursor);
    static_assert(sizeof(current_timing) == sizeof(uint64_t), "value must be saved as 64bits BE");
    output_as_big_endian(out, current_timing);

    // output number of events occuring at this timing
    const auto nb_events = get_nb_events_at_time(key_event_it, cursor_event_it,
						 end_key, end_cursor,
						 current_timing);

    static_assert(sizeof(nb_events) == 1, "value must be saved as 8bits BE");
    output_as_big_endian(out, nb_events);

    // output the graphical related event first (change svg_file then cursor pos)
    while ((cursor_event_it != end_cursor)
	   and (cursor_event_it->start_time == current_timing))
    {
      output_cursor_move_event(out, *cursor_event_it, current_svg_file);
      cursor_event_it++;
    }

    // output the key presses / key releases events
    while ((key_event_it != end_key) and
	   (key_event_it->time == current_timing))
    {
      output_key_event(out, key_event_it->data);
      key_event_it++;
    }
  }
}


void save_events_to_file(const std::string& output_filename,
			 const std::vector<key_event>& keyboard_events,
			 const std::vector<cursor_box_t>& cursor_boxes)
{
  std::ofstream file(output_filename,
		     std::ios::binary | std::ios::trunc | std::ios::out);

  if (not file.is_open())
  {
    throw std::runtime_error(std::string{"Error: failed to open "} + output_filename);
  }

  output_events_data(file, keyboard_events, cursor_boxes);
  file.close();
}
