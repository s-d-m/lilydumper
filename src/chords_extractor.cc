#include <algorithm>
#include <stdexcept>
#include "chords_extractor.hh"

std::vector<chord_t> get_chords(const std::vector<note_t>& notes)
{
  // sanity check: precondition notes must be sorted by starting time
  if (not std::is_sorted(notes.cbegin(), notes.cend(), [] (const auto& a, const auto& b) {
	return a.start_time < b.start_time;
      }))
  {
    throw std::logic_error("Error: notes should be sorted by starting time");
  }

  std::vector<chord_t> res;

  const auto nb_notes = notes.size();

  for (auto current_note = decltype(nb_notes){0};
       current_note < nb_notes;)
  {
    const auto start_time = notes[current_note].start_time;

    chord_t new_chord;
    new_chord.notes.push_back(notes[current_note]);

    // while the time start time doesn't change, add the note to the chord
    ++current_note;
    while ((current_note < nb_notes) and
	   (notes[current_note].start_time == start_time))
    {
      new_chord.notes.push_back(notes[current_note]);
      ++current_note;
    }

    res.emplace_back(std::move(new_chord));
  }

  // sanity check: post condition.  all notes must be part of a chord.
  // hence their must be the same number of notes in res than in the input.
  const auto nb_out_notes = std::accumulate(res.cbegin(), res.cend(), static_cast<decltype(res[0].notes.size())>(0),
					    [] (const auto accu, const auto& chord) {
					      return accu + chord.notes.size();
					    });

  if (nb_out_notes != nb_notes)
  {
    throw std::logic_error("Error while grouping notes into chords");
  }

  // sanity check: all notes in a chord must start at the same time
  if (std::any_of(res.cbegin(), res.cend(), [] (const auto& chord) {
	return std::any_of(chord.notes.cbegin(), chord.notes.cend(), [&] (const auto& note) {
	    return note.start_time != chord.notes[0].start_time;
	  });}))
  {
    throw std::logic_error("Error not all notes in a chord start at the same time as they should");
  }

  // sanity check: the chords must be sorted by starting time. And two
  // different chords must start at different times
  const auto nb_chords = res.size();
  for (decltype(res.size()) i = 0; i + 1 < nb_chords; ++i)
  {
    if (res[i].notes[0].start_time >= res[i + 1].notes[0].start_time)
    {
      throw std::logic_error("Error the chords are not sorted in strict ascending order");
    }
  }

  return res;
}
