#include <string>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <iterator>
#include "notes_file_extractor.hh"
#include "utils.hh"

static void extend_tied_notes(std::vector<note_t>& notes)
{
  // precondition, notes are sorted by time
  if (not std::is_sorted(std::begin(notes), std::end(notes), [] (const auto& a, const auto& b) {
	return a.start_time < b.start_time;
      }))
  {
    throw std::logic_error("Error: notes are not sorted by time");
  }

  auto it = notes.begin();
  const auto end = notes.end();
  while (it not_eq end)
  {
    const auto with_tie_note = std::find_if(it, end, [] (const auto& note) {
	return (note.id.find("#has-tie-attached=yes#") != std::string::npos) and note.is_played;
      });

    if (with_tie_note == end)
    {
      // no more notes with ties, finished
      it = end;
    }
    else
    {
      it = std::next(with_tie_note);

      // need to extend the duration time of this note.  Find the following
      // note_S_ having the same pitch/staff-number and starts when this one
      // finishes. Since several notes can be "chained" by ties, e.g. like
      // do4~do~do, the extension time must find the latest one of the chain.
      const auto pitch = with_tie_note->pitch;
      const auto staff_number = std::string{"#staff-number="}
				+ get_value_from_field(with_tie_note->id, "staff-number") + "#";

      bool chain_finished;
      do
      {
	const auto when_tie_finish = with_tie_note->stop_time;
	const auto next_note = std::find_if(std::next(with_tie_note), end, [&] (const auto& note) {
	    return (note.pitch == pitch) and (note.id.find(staff_number) != std::string::npos)
	            and (note.start_time == when_tie_finish);
	  });

	if (next_note != end)
	{
	  // important check has the loop is based on the assumption that no
	  // notes ends at the same time they starts. A note with a duration
	  // time of 0 would cause an infinite loop here.
	  if (next_note->stop_time == next_note->start_time)
	  {
	    throw std::logic_error("Error: a not has a duration of 0ns");
	  }

	  next_note->is_played = false;
	  with_tie_note->stop_time = next_note->stop_time;
	}

	chain_finished = ((next_note == end) or (next_note->id.find("#has-tie-attached=no#")));
      } while (not chain_finished);
    }
  }
}

std::vector<note_t> get_notes(const std::string& filename)
{
  std::ifstream file (filename, std::ios::in);
  if (! file.is_open() )
  {
    throw std::runtime_error(std::string{"Error: failed to open '"} + filename + "'");
  }

  std::vector<note_t> res;

  for (std::string line; std::getline(file, line); )
  {
    std::istringstream str (line);
    std::string line_type;
    std::string field1;
    uint64_t start_time;
    std::string field2;
    uint64_t stop_time;
    std::string field3;
    unsigned int bar_number;
    std::string field4;
    std::string id_str;

    str >> line_type
	>> field1
	>> start_time
	>> field2
	>> stop_time
	>> field3
	>> bar_number
	>> field4
	>> id_str;

    if (line_type != "note")
    {
      throw std::runtime_error(std::string{"Error: invalid line found in '"}
			       + filename + "'. Starts by '" + field1
			       + "' instead of either 'note'");
    }

    if (field1 != "start-time:")
    {
      throw std::runtime_error(std::string{"Error: invalid fieldname found in '"}
			       + filename + "'. found '" + field1
			       + "' instead of either 'start-time:'");
    }

    if (field2 != "stop-time:")
    {
      throw std::runtime_error(std::string{"Error: invalid fieldname found in '"}
			       + filename + "'. found '" + field2
			       + "' instead of 'stop-time:'");

    }

    if (field3 != "bar-number:")
    {
      throw std::runtime_error(std::string{"Error: invalid fieldname found in '"}
			       + filename + "'. found '" + field3
			       + "' instead of 'bar-number:'");

    }

    if (field4 != "id:")
    {
      throw std::runtime_error(std::string{"Error: invalid fieldname found in '"}
			       + filename + "'. found '" + field4
			       + "' instead of 'id:'");

    }

    res.emplace_back(note_t{
	.start_time = start_time,
	.stop_time = stop_time,
        .bar_number = static_cast<decltype(note_t::bar_number)>(bar_number),
	.pitch = static_cast<decltype(note_t::pitch)>(std::stoul(get_value_from_field(id_str, "pitch"))),
	.is_played = true, // set to true for now. second pass will set this value based on ties
	.id = std::move(id_str) }
      );
  }

  // res is not sorted now due to the current handling of grace notes.
  std::stable_sort(std::begin(res), std::end(res), [] (const auto& a, const auto& b) {
      return a.start_time < b.start_time;
    });

  extend_tied_notes(res);

  return res;
}
