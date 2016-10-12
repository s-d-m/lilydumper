#include <algorithm>
#include <stdexcept>
#include <string>
#include "cursor_boxes_extractor.hh"

static constexpr const char * const maybe_has_repeat_unfold_msg =
  "This can happen if the lilypond source file contains unfolded repeats, expand a variable twice.\n"
  "If this is the case, try editing the source file and replace the \"repeat unfold x {music}\" by \n"
  "copy-pasting the music x times, expand variables by hand and rerun the program";


// returns true iff the svg contains a note_head with the given id
static
bool has_note(const svg_file_t& svg, const std::string& id_str)
{
  return std::any_of(svg.note_heads.cbegin(), svg.note_heads.cend(), [&] (const auto& elt) {
      return elt.id == id_str;
    });
}

// return the indexes of all svg files containing a note_head with the same id as note.
// post condition, the output is sorted
static
std::vector<uint8_t> find_svg_files(const note_t& note,
				    const std::vector<svg_file_t>& svg_files)
{
  std::vector<uint8_t> res;

  const auto nb_svg = svg_files.size();
  if (nb_svg > std::numeric_limits<decltype(res)::value_type>::max())
  {
    throw std::runtime_error(std::string{"Error, this program can't handle more than "}
                             + std::to_string(static_cast<int>(std::numeric_limits<uint8_t>::max())) +
                             " svg files per music sheet");
  }

  for (decltype(res)::value_type i = 0; i < static_cast<decltype(i)>(nb_svg); ++i)
  {
    if (has_note(svg_files[i], note.id))
    {
      res.push_back(i);
    }
  }

  // sanity check
  if (not std::is_sorted(res.cbegin(), res.cend()))
  {
    throw std::logic_error("output vector should be sorted");
  }

  // sanity check: a note head should appear on at least one svg file
  if (res.empty())
  {
    throw std::runtime_error(std::string{"Error: note head with the following id couldn't be found in any svg file\n  not found id: "} + note.id);
  }

  return res;
}


// return the index of all svg files containing all the note
// E.g. if the notes id are [ "foo", "bar", "baz" ] and
// the function returns [ 0, 2, 3 ], it means that svg_files[0] contains
// a note head with if "foo", another one with id "bar", and and yet
// another one with id "baz". Same goes for svg_files[2], and svg_files[3]
//
// if svg_files[1] has a note with id "foo", and "bar", but is missing "baz"
// 1 won't appear in the results
static
std::vector<uint8_t> find_svg_files(const std::vector<note_t>& notes,
				    const std::vector<svg_file_t>& svg_files)
{
  if (notes.empty() or svg_files.empty())
  {
    throw std::logic_error("Error: invalid parameters");
  }

  std::vector<uint8_t> res;

  res = find_svg_files(notes[0], svg_files);

  const auto nb_notes = notes.size();
  for (unsigned int i = 1; i < static_cast<decltype(i)>(nb_notes); ++i)
  {
    const auto current_svg = find_svg_files(notes[i], svg_files);

    // tmp = res intersection current_svg
    decltype(res) tmp;
    std::set_intersection(res.cbegin(), res.cend(),
			  current_svg.cbegin(), current_svg.cend(),
			  std::back_inserter(tmp));

    res = std::move(tmp); // res <- tmp
  }

  return res;
}


// return the index of the file containing all the notes
// E.g. if the notes id are [ "foo", "bar", "baz" ] and
// the function returns 2, it means that svg_files[2] contains
// a note head with if "foo", another one with id "bar", and and yet
// another one with id "baz".
//
// There must be one and only one svg file containing all the given
// notes.  The notes parameters must be the notes of a single
// chord. Due to the context of the application, they must all be
// present in at least one page of the music sheet. Also, the music
// sheets must have been generated in a way that a single chord don't
// appear twice. In other words, each note must have a uniq id. As a
// consequence, a chord identified by its notes can't be found twice.
static uint8_t find_svg_pos(const std::vector<note_t>& notes,
			    const std::vector<svg_file_t>& svg_files)
{
  // sanity check: pre-condition
  if (notes.empty() or svg_files.empty())
  {
    throw std::logic_error("Error: invalid parameters");
  }

  // sanity check: pre-condition, the notes must be part of a
  // chord. Therefore, they must all start at the same time.
  const auto start_time = notes[0].start_time;
  if (std::any_of(notes.cbegin(), notes.cend(), [=] (const auto& a) {
	return a.start_time != start_time;
      }))
  {
    throw std::runtime_error("Error: all notes of a chord must start at the same time");
  }

  const auto indexes = find_svg_files(notes, svg_files);

  // sanity check: post-condition, the notes must all appear in at least one svg file
  if (indexes.empty())
  {
    std::string err_msg = "Error: the notes with the following ids could not be found all in the same svg file";
    for (const auto& note : notes)
    {
      err_msg += "\n  " + note.id;
    }
    throw std::runtime_error(err_msg);
  }

  // sanity check: post-condition, a note with a specific id must
  // appear only once in the whole music sheet, therefore in only one
  // svg file
  if (indexes.size() != 1)
  {
    std::string err_msg = "Error: notes with the following IDs\n";
    for (const auto& note : notes)
    {
      err_msg += "  " + note.id + "\n";
    }
    err_msg += "appears in in the following files (they should appear in only one file)\n";
    for (const auto& index : indexes)
    {
      err_msg += "  " + svg_files[index].filename.string() + "\n";
    }
    err_msg += maybe_has_repeat_unfold_msg;
    throw std::runtime_error(err_msg);
  }

  return indexes[0];
}

// return the note head in the svg file with that specific id
static note_head_t get_note_head(const std::string& id,
				 const svg_file_t svg_file)
{
  // sanity check: precondition there must be one, and only note
  // note_head with that specific id
  const auto nb_candidates = std::count_if(svg_file.note_heads.cbegin(),
					   svg_file.note_heads.cend(),
					   [&] (const auto& elt) { return elt.id == id; });
  if (nb_candidates != 1)
  {
    if (nb_candidates == 0)
    {
      throw std::runtime_error(std::string{"Error: a note head with id "} + id + " could not be found in a svg file");
    }
    else
    {
      throw std::runtime_error(std::string{"Error: a note head with id "} + id + " has been found several times (" +
			       std::to_string(nb_candidates) + ") in the svg file " + svg_file.filename.c_str() + "\n" +
                               maybe_has_repeat_unfold_msg);
    }
  }

  return *std::find_if(svg_file.note_heads.cbegin(),
		       svg_file.note_heads.cend(),
		       [&] (const auto& elt) { return elt.id == id; });
}


// return the subset of candidate_systems that really contains x, y
static std::vector<uint8_t> find_system_with_point(const svg_file_t& svg_file,
						   const std::vector<uint8_t>& candidate_systems,
						   uint32_t x,
						   uint32_t y)
{
  // sanity check: pre-condition y must be in the top/bottom skyline range of all candidate_systems
  for (const auto candidate : candidate_systems)
  {
    const auto top_staff = svg_file.systems[ candidate ].first;
    const auto bottom_staff = svg_file.systems[ candidate ].last;

    const auto top_skyline = svg_file.staves[ top_staff ].top_skyline;
    const auto bottom_skyline = svg_file.staves[ bottom_staff ].bottom_skyline;

    if (not ((top_skyline <= y) and (y <= bottom_skyline)))
    {
      throw std::runtime_error("precondition failed: candidate system is wrong");
    }
  }


  std::vector<uint8_t> res;
  for (const auto candidate : candidate_systems)
  {
    const auto top_staff = svg_file.systems[ candidate ].first;
    const auto bottom_staff = svg_file.systems[ candidate ].last;

    const auto& full_top_skyline = svg_file.staves[ top_staff ].full_top_skyline;
    const auto& full_bottom_skyline = svg_file.staves[ bottom_staff ].full_bottom_skyline;

    // find the first (normally only) segment on top on x, y
    const auto top_segment = std::find_if(full_top_skyline.cbegin(), full_top_skyline.cend(), [=] (const auto& elt) {
	return (elt.x1 <= x) and (x <= elt.x2) and (elt.y <= y);
      });

    // find the first (normally only) segment below x, y
    const auto bottom_segment = std::find_if(full_bottom_skyline.cbegin(), full_bottom_skyline.cend(), [=] (const auto& elt) {
	return (elt.x1 <= x) and (x <= elt.x2) and (y <= elt.y);
      });

    if ((top_segment != full_top_skyline.cend()) and (bottom_segment != full_bottom_skyline.cend()))
    {
      res.push_back(candidate);
    }
  }


  // sanity check: post-condition, there can't be more candidate at
  // the end of this function than there was at the beginning.
  if (res.size() > candidate_systems.size())
  {
    throw std::logic_error("postcondition failed: there can't be more possibilities after filtering");
  }

  return res;
}


static uint8_t find_system_with_point(const svg_file_t& svg_file,
				      uint32_t x,
				      uint32_t y)
{
  if (svg_file.systems.empty())
  {
    throw std::logic_error("Error: music sheet with no systems");
  }

  std::vector<uint8_t> res;

  const auto last = static_cast<uint8_t>(svg_file.systems.size());
  for (uint8_t i = 0; i < last; ++i)
  {
    const auto& this_system = svg_file.systems[i];
    const auto top_system = svg_file.staves[ this_system.first ].top_skyline;
    const auto bottom_system = svg_file.staves[ this_system.last ].bottom_skyline;

    if ((top_system <= y) and (y <= bottom_system))
    {
      res.push_back(i);
    }
  }

  auto nb_candidates = res.size();
  if (nb_candidates > 1)
  {
    // it is possible that two systems overlap when considering the
    // top skyline and bottom skyline only as their limits.  In such
    // a case, one has to look in more details using the full
    // top/bottom skyline.

    auto tmp = find_system_with_point(svg_file, std::move(res), x, y);
    res = std::move(tmp);
    nb_candidates = res.size();
  }

  if (nb_candidates != 1)
  {
    if (nb_candidates == 0)
    {
      throw std::runtime_error("Error, unable to find a system containing a cursor box");
    }
    else
    {
      throw std::runtime_error("Error, too many systems contains the same cursor box");
    }
  }

  return res[0];
}

static cursor_box_t get_cursor_box(const chord_t& chord,
				   const std::vector<svg_file_t>& svg_files)
{
  const auto& notes = chord.notes;

  // sanity check: pre-condition
  if (notes.empty() or svg_files.empty())
  {
    throw std::logic_error("Error: invalid parameters");
  }

  // sanity check: pre-condition, the notes must be part of a
  // chord. Therefore, they must all start at the same time.
  const auto start_time = notes[0].start_time;
  if (std::any_of(notes.cbegin(), notes.cend(), [=] (const auto& a) {
	return a.start_time != start_time;
      }))
  {
    throw std::runtime_error("Error: all notes of a chord must start at the same time");
  }

  const auto svg_pos = find_svg_pos(notes, svg_files);
  const auto& svg_file = svg_files[svg_pos];

  auto min_left = std::numeric_limits<decltype(cursor_box_t::left)>::max();
  auto max_right = std::numeric_limits<decltype(cursor_box_t::right)>::min();
  auto min_top = std::numeric_limits<decltype(cursor_box_t::left)>::max();
  auto max_bottom = std::numeric_limits<decltype(cursor_box_t::right)>::min();

  const auto first_note_head = get_note_head(notes[0].id, svg_file);
  const auto first_bar_number = first_note_head.bar_number;

  for (const auto& note : notes)
  {
    const auto head = get_note_head(note.id, svg_file);
    min_left = std::min(min_left, head.left);
    max_right = std::max(max_right, head.right);
    min_top = std::min(min_top, head.top);
    max_bottom = std::max(max_bottom, head.bottom);

    // sanity check, since all notes in a chord starts at the same time,
    // they must all share the same bar number
    if (head.bar_number != first_bar_number)
    {
      throw std::runtime_error(std::string{"Error: all notes in a chord must have the same bar number\n"} +
			       "in svg file [" + svg_file.filename.c_str() + "]\n"
			       "notes with bar number  " + std::to_string(static_cast<unsigned>(first_bar_number)) +
			       " and id " + first_note_head.id + "\n" +
			       "and the one within bar " + std::to_string(static_cast<unsigned>(head.bar_number)) +
			       " and id " + head.id + "\n" +
			       "appear in the same chord\n" +
			       "\n"
			       "This error occurs when the source file contains a repeat in a voice,"
			       " but not in all of them.\n"
			       "If one voice uses a repeat, but not on the second one, lilydumper considers"
			       " that at some point it has to play the first note of the part to repeat in"
			       " the first voice, and the first note right after the repeat part in the second"
			       " voice at the same time. This is obviously wrong and the lilydumper detects"
			       " this and issues this error.\n"
			       "One way to fix it is to edit the source file and use repeats on every voices,"
			       " and run lilydumper again");
    }
  }

  const auto system = find_system_with_point(svg_file,
					     (min_left + max_right) / 2,
					     (min_top + max_bottom) / 2);

  const auto first_staff = svg_file.systems[ system ].first;
  const auto last_staff = svg_file.systems[ system ].last;

  const auto system_top = svg_file.staves[ first_staff ].top_skyline;
  const auto system_bottom = svg_file.staves[ last_staff ].bottom_skyline;

  const cursor_box_t res = {
      .left = min_left,
      .right = max_right,
      .top = system_top,
      .bottom = system_bottom,
      .start_time = notes[0].start_time,
      .svg_file_pos = svg_pos,
      .system_number = system,
      .bar_number = first_bar_number };

  // sanity check: a cursor box must be at least 1unit high and wide
  if ((res.left >= res.right) or (res.top >= res.bottom))
  {
    throw std::runtime_error("Error cursor box found is invalid.");
  }

  return res;
}

// returns a cursor for each chord. chords[ x ] -> res[ x ]
std::vector<cursor_box_t> get_cursor_boxes(const std::vector<chord_t>& chords,
					   const std::vector<svg_file_t>& svg_files)
{
  std::vector<cursor_box_t> res;

  for (const auto& chord : chords)
  {
    res.emplace_back( get_cursor_box(chord, svg_files) );
  }

  return res;
}
