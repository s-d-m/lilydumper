#include <sys/types.h> // for get file size
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
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
			      const std::vector<bar_num_event_t>::const_iterator& bar_num_event_it,
			      const std::vector<key_event>::const_iterator& end_key,
			      const std::vector<cursor_box_t>::const_iterator& end_cursor,
			      const std::vector<bar_num_event_t>::const_iterator& end_bar_num,
			      const uint64_t current_timing_event,
			      const decltype(cursor_box_t::svg_file_pos) current_svg_file)
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
			     std::to_string(static_cast<unsigned>(std::numeric_limits<uint8_t>::max())) +
			     "events occuring at the same time");
    }

    ++nb_key_events;
  }

  unsigned int nb_cursor_events = 0;
  static_assert(std::numeric_limits<decltype(nb_cursor_events)>::max() >= std::numeric_limits<uint8_t>::max(),
		"nb_events can constain key events and cursor events. Therefore the maximum number of cursor_events must be at least as big as the number of all events so avoid wrapping when uncessary");

  // in case there is a cursor change, one has to check if the next cursor is on
  // the same page or not. Turn pages are written off as separate events,
  // therefore it must be counted separately than cursor_events
  unsigned int nb_page_turn_event = 0;

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

    if (it->svg_file_pos != current_svg_file)
    {
      nb_page_turn_event++;
    }
  }

  unsigned int nb_bar_num_events = 0;
  static_assert(std::numeric_limits<decltype(nb_bar_num_events)>::max() >= std::numeric_limits<uint8_t>::max(),
		"nb_events can constain nb_bar_num_events. Therefore the maximum number of bar_num_events must be at least as big as the number of all events so avoid wrapping when uncessary");

  for (auto it = bar_num_event_it;
       (it != end_bar_num) and (it->time == current_timing_event);
       ++it)
  {
    ++nb_bar_num_events;

    // sanity check
    if (nb_bar_num_events > 1)
    {
      throw std::logic_error("Error: can't change the bar number twice (or more) at the same.");
    }
  }

  const auto res = nb_cursor_events +
		   nb_key_events +
                   nb_bar_num_events +
		   nb_page_turn_event;

  // sanity check
  if (res > std::numeric_limits<uint8_t>::max())
  {
    throw std::logic_error(std::string{"Error: can't handle more than "} +
			   std::to_string(static_cast<unsigned>(std::numeric_limits<uint8_t>::max())) +
			   "events occuring at the same time");
  }

  return static_cast<uint8_t>(res);
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
						   const std::vector<bar_num_event_t>::const_iterator& bar_num_event_it,
						   const std::vector<key_event>::const_iterator& end_key,
						   const std::vector<cursor_box_t>::const_iterator& end_cursor,
						   const std::vector<bar_num_event_t>::const_iterator& end_bar_num)
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

  if (bar_num_event_it != end_bar_num)
  {
    res = std::min(res, bar_num_event_it->time);
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
void output_bar_num_event(std::ofstream& out,
			  const bar_num_event_t& bar_num_ev)
{
  static_assert(sizeof(event_type::set_bar_number) == 1, "an event type should be one byte");
  output_as_big_endian(out, event_type::set_bar_number);

  static_assert(sizeof(bar_num_ev.bar_number) == 2, "bar number should be two bytes");
  output_as_big_endian(out, bar_num_ev.bar_number);
}

static
void output_events_data(std::ofstream& out,
			const std::vector<key_event>& keyboard_events,
			const std::vector<cursor_box_t>& cursor_boxes,
			const std::vector<bar_num_event_t>& bar_num_events)
{
  // sanity check: inputs must be sorted by time
  if (not std::is_sorted(keyboard_events.cbegin(), keyboard_events.cend(), [] (const auto& a, const auto&b) {
	return a.time < b.time;
      }))
  {
    throw std::invalid_argument("Error: keyboard events should be sorted by time");
  }

  // sanity check: inputs must be sorted by time
  if (not std::is_sorted(cursor_boxes.cbegin(), cursor_boxes.cend(), [] (const auto& a, const auto&b) {
	return a.start_time < b.start_time;
      }))
  {
    throw std::invalid_argument("Error: cursor events should be sorted by time");
  }

  // sanity check: inputs must be sorted by time
  if (not std::is_sorted(bar_num_events.cbegin(), bar_num_events.cend(), [] (const auto& a, const auto&b) {
	return a.time < b.time;
      }))
  {
    throw std::invalid_argument("Error: bar number events should be sorted by time");
  }



  auto key_event_it = keyboard_events.cbegin();
  auto cursor_event_it = cursor_boxes.cbegin();
  auto bar_num_event_it = bar_num_events.cbegin();

  const auto end_key = keyboard_events.cend();
  const auto end_cursor = cursor_boxes.cend();
  const auto end_bar_num = bar_num_events.cend();

  auto current_svg_file = std::numeric_limits<decltype(cursor_box_t::svg_file_pos)>::max();

  // the parser/reader needs to know when to stop reading events and when to
  // start reading what comes next (svg files). One way to do so is to have a
  // special "event type" that encodes "end of events". Another way is to add a
  // field before the start of the group of events which tells how many of them
  // there is

  // The choice here has been to use the second method: tell how many numbers of
  // group there is before dumping them. Sadly at this point, the software
  // doesn't know it yet. Therefore let's leave 8 bytes in the output file for
  // now, and update it when the program can tell how many group of events there
  // is.

  const auto nb_groups_pos = out.tellp();

  uint64_t nb_groups_of_events = 0;
  output_as_big_endian(out, nb_groups_of_events);

  // while there still is at least one event to process
  while ((key_event_it != end_key) or
	 (cursor_event_it != end_cursor) or
	 (bar_num_event_it != end_bar_num))
  {
    nb_groups_of_events++;

    // output timing
    const auto current_timing = get_current_event_timing(key_event_it,
							 cursor_event_it,
							 bar_num_event_it,
							 end_key,
							 end_cursor,
							 end_bar_num);
    static_assert(sizeof(current_timing) == sizeof(uint64_t), "value must be saved as 64bits BE");
    output_as_big_endian(out, current_timing);

    // output number of events occuring at this timing
    const auto nb_events = get_nb_events_at_time(key_event_it,
						 cursor_event_it,
						 bar_num_event_it,
						 end_key,
						 end_cursor,
						 end_bar_num,
						 current_timing,
						 current_svg_file);

    static_assert(sizeof(nb_events) == 1, "value must be saved as 8bits BE");
    output_as_big_endian(out, nb_events);

    // output the graphical related event first (change svg_file then cursor pos)
    while ((cursor_event_it != end_cursor)
	   and (cursor_event_it->start_time == current_timing))
    {
      output_cursor_move_event(out, *cursor_event_it, current_svg_file);
      cursor_event_it++;
    }

    // output the bar num event changes
    while ((bar_num_event_it != end_bar_num) and
	   (bar_num_event_it->time == current_timing))
    {
      output_bar_num_event(out, *bar_num_event_it);
      bar_num_event_it++;
    }

    // output the key presses / key releases events
    while ((key_event_it != end_key) and
	   (key_event_it->time == current_timing))
    {
      output_key_event(out, key_event_it->data);
      key_event_it++;
    }
  }

  // now that we know how many group of events there is, let's write it at the
  // right place in the file.
  out.seekp(nb_groups_pos);
  output_as_big_endian(out, nb_groups_of_events);

  // don't forget to set the output position indicator back to the end.
  out.seekp(0, std::ios_base::end);
}

static
void output_staff_num_mapping(std::ofstream& file,
			      const std::vector<std::string>& staff_num_mapping)
{
  file << static_cast<uint8_t>(staff_num_mapping.size());

  for (const auto& elt : staff_num_mapping)
  {
    file << elt
	 << static_cast<uint8_t>( 0 );
  }
}

static
void output_svg_files(std::ofstream& file,
		      const std::vector<fs::path>& svg_filenames)
{
  output_as_big_endian(file, static_cast<uint16_t>(svg_filenames.size()));
  for (const auto& filename : svg_filenames)
  {
    // get file size: http://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c

    // While not necessarily the most popular method, I've heard that the ftell,
    // fseek method may not always give accurate results in some
    // circumstances. Specifically, if an already opened file is used and the
    // size needs to be worked out on that and it happens to be opened as a text
    // file, then it's going to give out wrong answers.

    struct stat stat_buf;
    const auto rc = stat(filename.c_str(), &stat_buf);
    if (rc != 0)
    {
      throw std::runtime_error(std::string{"Error, failed to get file size for "} + filename.string());
    }
    output_as_big_endian(file, static_cast<uint32_t>(stat_buf.st_size));

    std::ifstream src(filename, std::ios::in | std::ios::binary);
    file << src.rdbuf();
  }
}


void save_to_file(const std::string& output_filename,
		  const std::vector<key_event>& keyboard_events,
		  const std::vector<cursor_box_t>& cursor_boxes,
		  const std::vector<bar_num_event_t>& bar_num_events,
		  const std::vector<std::string>& staff_num_mapping,
		  const std::vector<fs::path>& svg_filenames)
{
  std::ofstream file(output_filename,
		     std::ios::binary | std::ios::trunc | std::ios::out);

  if (not file.is_open())
  {
    throw std::runtime_error(std::string{"Error: failed to open "} + output_filename);
  }

  // magic number: LPYP
  file << static_cast<uint8_t>( 'L' )
       << static_cast<uint8_t>( 'P' )
       << static_cast<uint8_t>( 'Y' )
       << static_cast<uint8_t>( 'P' )
    // version number
       << static_cast<uint8_t>( 0 );

  output_staff_num_mapping(file, staff_num_mapping);
  output_events_data(file, keyboard_events, cursor_boxes, bar_num_events);
  output_svg_files(file, svg_filenames);
  file.close();
}
