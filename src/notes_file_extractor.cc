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
	  // important check:  the loop is based on the assumption that no
	  // notes end at the same time as they start. A note with a duration
	  // time of 0 would cause an infinite loop here otherwise.
	  if (next_note->stop_time == next_note->start_time)
	  {
	    throw std::logic_error("Error: a note has a duration of 0ns");
	  }

	  next_note->is_played = false;
	  with_tie_note->stop_time = next_note->stop_time;
	}

	chain_finished = ((next_note == end) or (next_note->id.find("#has-tie-attached=no#") != std::string::npos));
      } while (not chain_finished);
    }
  }
}

// set the correct timing of grace notes in  place
static void fix_grace_notes(std::vector<note_t>& notes)
{
  // there are three different possible case for grace notes:
  // - the music sheet starts with grace notes    (corner case, but can really happen)
  // - grace notes are "wrapped" by normal ones   (expected normal case)
  // - the music sheet ends with grace notes      (corner case, unsure if it makes sense "musically" speaking. Let's handle them anyway)


  const auto nb_notes = notes.size();

  // let's treat corner case one: music sheet starting by grace notes
  decltype(notes.size()) first_normal_note = 0;
  while ((first_normal_note < nb_notes) and
	 (notes[first_normal_note].id.find("#is-grace-note=yes#") != std::string::npos))
  {
    first_normal_note++;
  }

  // let's say all grace note from the very beginning will last 1/8 of a second.
  // totally arbitrary value!
  constexpr auto grace_note_length = decltype(note_t::start_time){ 125000000 }; // unit is nanoseconds

  // first_normal_note is either out of range (extreme case of a music
  // sheet containing only grace notes), or points to the first normal note.
  //
  // in case it only has grace notes, first_normal_note will equal to
  // nb_notes, thus the following code will be correct as it will
  // treat all notes.
  //
  // in case it doesn't start by grace notes, first_normal_note will
  // be 0 and the following loop won't be entered, so still correct.
  //
  // in case there are some grace note at the start, and eventually a
  // normal note, this is exactly what it is supposed to do.
  for (decltype(first_normal_note) i = 0; i < first_normal_note; ++i)
  {
    notes[i].start_time = i * grace_note_length;
    notes[i].stop_time = (i + 1) * grace_note_length;
  }

  // for all notes from first_normal_note, up to the end, delays them
  // by the time used by the first grace notes
  const auto delay_timing = grace_note_length * first_normal_note;

  // following if is uncessary. in the case there is no delay (no
  // grace notes at start of music sheet which is a very common case),
  // the for loop would just add 0 so not modifying the values.  The
  // goal of the if is thus to avoid spending a lot of CPU time in the
  // very common case by avoiding going through the vector.
  if (delay_timing != 0)
  {
    for (auto i = first_normal_note; i < nb_notes; ++i)
    {
      notes[i].start_time += delay_timing;
      notes[i].stop_time += delay_timing;
    }
  }

  // following is to treat the normal case (grace notes embraced by normal ones)
  decltype(notes.size()) current_normal_note = first_normal_note;
  while (current_normal_note < nb_notes)
  {
    // find the next grace note
    auto grace_pos = current_normal_note + 1;
    while ((grace_pos < nb_notes) and
	   (notes[grace_pos].id.find("#is-grace-note=yes#") == std::string::npos))
    {
      grace_pos++;
    }

    // at this point we are either on a grace note, or we finished processing.

    // if grace_pos "points" to a grace note, the following will look at the next
    // normal note.
    //
    // if we already finished processing, next_normal will be out-of-bounds.
    auto next_normal = grace_pos + 1; // normal mean non-grace note here
    while ((next_normal < nb_notes) and
	   (notes[next_normal].id.find("#is-grace-note=yes#") != std::string::npos))
    {
      next_normal++;
    }

    // at this point if we are still in the range, do some work.
    // There is one corner case here: a music sheet can finish with
    // grace notes. Therefore it is possible that grace_pos is in the
    // range but not next_normal. Therefore use next_normal instead of
    // grace_pos in the comparison.
    //
    if (next_normal < nb_notes)
    {
      // find the previous normal note.  the fix for grace notes
      // embraced by two normal ones is simple. Find the duration
      // between the two normal ones. Each grace note will last the
      // same amount of time and while all start after previous_normal
      // and last before next_normal

      const auto previous_normal = grace_pos - 1; // we are sure this is valid as we started grace_pos at current_normal_note + 1
      const auto nb_grace_notes = next_normal - grace_pos;
      const auto nb_slots = nb_grace_notes + 1;
      const auto diff_time = notes[next_normal].start_time - notes[previous_normal].start_time;

      for (auto i = decltype(nb_grace_notes){ 0 }; i < nb_grace_notes; ++i)
      {
	notes[grace_pos + i].start_time = notes[previous_normal].start_time + ((diff_time * (i + 1)) / nb_slots);
	notes[grace_pos + i].stop_time = notes[previous_normal].start_time + ((diff_time * (i + 2)) / nb_slots);
      }
    }

    current_normal_note = next_normal;
  }

  // let's fix the last corner case, grace notes at the end of the
  // music sheet with at least one normal note before (otherwise it
  // would mean music sheet made only of grace notes)
  auto last_normal = nb_notes;
  bool finished = false;
  while ((last_normal > 0) and (not finished))
  {
    last_normal--; // we started at nb_notes (so off by one to avoid
		   // problems when comparing with unsigned). In case
		   // the vector were empty ...
    finished = (notes[last_normal].id.find("#is-grace-note=yes#") == std::string::npos);
  }

  for (auto i = last_normal + 1; i < nb_notes; ++i)
  {
    notes[i].start_time = notes[last_normal].stop_time + grace_note_length * (i - last_normal);
    notes[i].stop_time = notes[last_normal].stop_time + grace_note_length * (i + 1 - last_normal);
  }
}


static
void fix_overlapping_key_presses(std::vector<note_t>& notes)
{
  // it can happen (though rarely) that on the music sheet, some keys
  // appear to "overlap". In other words, that a key must - according
  // to the music sheet - be pressed, while it is already pressed.

  // as an example, this happens in "Für Elise", on measure 30. The Do
  // (pitch = 72) is a do4 and there are these small grace notes
  // supposed to be played while holding this do4 (72), ... that also
  // contains a do (72).

  // the algorithm here is quite simple, when two key_down overlap in
  // time, shorten the first one so it finish at the exact same time
  // the second one starts. There will be a next step that will
  // separate the release/pressed events happening at the exact same
  // time

  for (auto i = decltype(notes.size()){0}; i < notes.size(); ++i)
  {
    for (auto j = i + 1; (j < notes.size()) and (notes[j].start_time <= notes[i].stop_time); ++j)
    {
      if (notes[j].pitch == notes[i].pitch)
      {
	notes[i].stop_time = notes[j].start_time;
      }
    }
  }
}


std::vector<note_t> get_notes(const fs::path& filename)
{
  std::ifstream file (filename, std::ios::in);
  if (! file.is_open() )
  {
    throw std::runtime_error(std::string{"Error: failed to open '"} + filename.c_str() + "'");
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
    std::string id_str;

    str >> line_type
	>> field1
	>> start_time
	>> field2
	>> stop_time
	>> field3
	>> id_str;

    if (line_type != "note")
    {
      throw std::runtime_error(std::string{"Error: invalid line found in '"}
			       + filename.c_str() + "'. Starts by '" + field1
			       + "' instead of 'note'");
    }

    if (field1 != "start-time:")
    {
      throw std::runtime_error(std::string{"Error: invalid fieldname found in '"}
			       + filename.c_str() + "'. found '" + field1
			       + "' instead of 'start-time:'");
    }

    if (field2 != "stop-time:")
    {
      throw std::runtime_error(std::string{"Error: invalid fieldname found in '"}
			       + filename.c_str() + "'. found '" + field2
			       + "' instead of 'stop-time:'");

    }

    if (field3 != "id:")
    {
      throw std::runtime_error(std::string{"Error: invalid fieldname found in '"}
			       + filename.c_str() + "'. found '" + field3
			       + "' instead of 'id:'");

    }

    const auto pitch = std::stoul(get_value_from_field(id_str, "pitch"));
    if ((pitch < pitch_t::la_0) or (pitch > pitch_t::do_8))
    {
      throw std::logic_error("Error: note is not valid for keyboard");
    }

    res.emplace_back(note_t{
	.start_time = start_time,
	.stop_time = stop_time,
	.pitch = static_cast<decltype(note_t::pitch)>(pitch),
	.is_played = true, // set to true for now. second pass will set this value based on ties
	.staff_number = static_cast<decltype(note_t::staff_number)>(std::stoul(get_value_from_field(id_str, "staff-number"))),
	.id = std::move(id_str) }
      );
  }

  fix_grace_notes(res);
  extend_tied_notes(res);
  fix_overlapping_key_presses(res);

  // sanity check: post condition the array must be sorted by time
    // res is not sorted now due to the current handling of grace notes.
  if (not std::is_sorted(std::begin(res), std::end(res), [] (const auto& a, const auto& b) {
	return a.start_time < b.start_time;
      }))
  {
    throw std::logic_error("The notes should be sorted by time now.");
  }

  return res;
}
