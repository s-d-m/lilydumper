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
      const auto staff_number = with_tie_note->staff_number;

      bool chain_finished;
      do
      {
	const auto when_tie_finish = with_tie_note->stop_time;
	const auto next_note = std::find_if(std::next(with_tie_note), end, [&] (const auto& note) {
	    return (note.pitch == pitch) and (note.staff_number == staff_number)
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

  // grace note length is (29 / 128) * (60 / 100) seconds to match
  // midi files produced by lilypond -> (29 * 60 * 1'000'000'000) / (128 * 100)
  constexpr auto grace_note_length = decltype(note_t::start_time){ 135937500 }; // unit is nanoseconds

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
      // However, the length of a grace note can't be bigger than grace_note_length
      // If the duration between the two normal notes is too big that the grace notes
      // will have to be limited to grace_note_length, then the grace notes will be
      // placed at the end of the interval, that is closer to the next normal note.

      const auto previous_normal = grace_pos - 1; // we are sure this is valid as we started grace_pos at current_normal_note + 1
      const auto nb_grace_notes = next_normal - grace_pos;
      const auto nb_slots = nb_grace_notes + 1;
      const auto diff_time = notes[next_normal].start_time - notes[previous_normal].start_time;
      const auto grace_time = std::min(grace_note_length, (diff_time / nb_slots));

      for (auto i = decltype(nb_grace_notes){ 0 }; i < nb_grace_notes; ++i)
      {
	notes[grace_pos + i].start_time = notes[next_normal].start_time - ((nb_grace_notes - i) * grace_time);
	notes[grace_pos + i].stop_time = notes[next_normal].start_time - ((nb_grace_notes - i - 1) * grace_time);
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
  const auto nr_notes = notes.size();
  for (auto i = decltype(nr_notes){0}; i < nr_notes; ++i)
  {
    for (auto j = i + 1; (j < nr_notes) and (notes[j].start_time <= notes[i].stop_time); ++j)
    {
      if (notes[j].pitch == notes[i].pitch)
      {
	notes[i].stop_time = notes[j].start_time;
      }
    }
  }

  // It is possible that due to the fixing made right above, a note gets to get a null played time,
  // i.e. that its stop_time now coincide with the start time. One can get an example of this with
  // the following snippet:
  //
  // \version "2.8.5"
  //
  // \score {
  //   \new PianoStaff \with{systemStartDelimiter = #'SystemStartBracket } <<
  //     \new Staff = "upper" { g }
  //     \new Staff = "lower" { \clef bass g}
  //   >>
  // }
  //
  // These null time notes need then to be removed.

  size_t i = 0;
  while (i < notes.size())
  {
    if (notes[i].start_time == notes[i].stop_time)
    {
      const auto it_pos = notes.begin() + static_cast<std::vector<key_event>::difference_type>(i);
      notes.erase(it_pos);
    }
    else
    {
      ++i;
    }
  }

}

std::vector<note_t> get_unprocessed_notes(const fs::path& filename)
{
  std::ifstream file (filename, std::ios::in);
  if (! file.is_open() )
  {
    throw std::runtime_error(std::string{"Error: failed to open '"} + filename.c_str() + "'");
  }

  std::vector<note_t> res;

  unsigned int current_line = 1;
  for (std::string line; std::getline(file, line); ++current_line)
  {
    std::istringstream str (line);
    std::string line_type;
    std::array<std::string, 4> fields;
    uint64_t start_time;
    uint64_t stop_time;
    uint64_t staff_number;

    str >> line_type
	>> fields[0]
	>> start_time
	>> fields[1]
	>> stop_time
	>> fields[2]
	>> staff_number
	>> fields[3];

    const decltype(fields) expected_fields = { { "start-time:", "stop-time:", "staff-number:", "id:"} };

    if (line_type != "note")
    {
      throw std::runtime_error(std::string{"Error in file '"} + filename.c_str() + "' at line " + std::to_string(current_line) + "\n"
			       + "  Line starts by '" +  line_type + "' instead of 'note'");
    }

    for (unsigned int i = 0; i < fields.size(); ++i)
    {
      if (fields[i] != expected_fields[i])
      {
	throw std::runtime_error(std::string{"Error in file '"} + filename.c_str() + "' at line " + std::to_string(current_line) + "\n"
				 "  Expected field name: " + expected_fields[i] + "\n"
				 "  Got: " + fields[i] );
      }
    }

    const std::string id_str (line.substr(line.find(expected_fields[expected_fields.size() - 1]) + 4)); // + 4 for strlen("id: ")
    if (id_str.find("#origin=") != 0)
    {
      throw std::runtime_error(std::string{"Error in file '"} + filename.c_str() + "' at line " + std::to_string(current_line) + "\n"
			       " the id does not start by '#origin='");
    }

    if (id_str.empty() or (id_str.back() != '#'))
    {
      throw std::runtime_error(std::string{"Error in file '"} + filename.c_str() + "' at line " + std::to_string(current_line) + "\n"
			       " the id does not end by '#'");
    }

    const auto pitch = std::stoul(get_value_from_field(id_str, "pitch"));
    if ((pitch < pitch_t::la_0) or (pitch > pitch_t::do_8))
    {
      throw std::runtime_error(std::string{"Error in file '"} + filename.c_str() + "' at line " + std::to_string(current_line) + "\n"
			       "  note with value " + std::to_string(pitch) + " and id " + id_str + " is not valid for keyboard.\n"
			       "  Should be between la_0 (" + std::to_string(static_cast<int>(pitch_t::la_0)) +
			       ") and do_8 (" + std::to_string(static_cast<int>(pitch_t::do_8)) + ")");
    }

    const auto is_transparent_note = [] (const std::string& id) {
      return id.find("#is-transparent=yes#") != std::string::npos;
    };

    if (not is_transparent_note(id_str))
    {
      res.emplace_back(note_t{
	  .start_time = start_time,
	    .stop_time = stop_time,
	    .pitch = static_cast<decltype(note_t::pitch)>(pitch),
	    .is_played = true, // set to true for now. second pass will set this value based on ties
	    .staff_number = static_cast<decltype(note_t::staff_number)>(staff_number),
	    .id = std::move(id_str) }
	);
    }
  }

  return res;
}

std::vector<note_t> get_processed_notes(const std::vector<note_t>& unprocessed_notes)
{
  auto res = unprocessed_notes;

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
