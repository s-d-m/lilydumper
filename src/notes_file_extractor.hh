#ifndef NOTES_FILE_EXTRACTOR_HH_
#define NOTES_FILE_EXTRACTOR_HH_

#include <vector>
#include <cstdint>
#include <string>

#define OCTAVE(X) \
  do_##X,	  \
  do_diese##X,	  \
  re_##X,	  \
  re_diese_##X,   \
  mi_##X,	  \
  fa_##X,	  \
  fa_diese_##X,   \
  sol_##X,	  \
  sol_diese_##X,  \
  la_##X,	  \
  la_diese_##X,	  \
  si_##X	  \

enum pitch_t : uint8_t
{
  /* scale 0 */
  la_0 = 21,
  la_diese_0,
  si_0,

  OCTAVE(1),
  OCTAVE(2),
  OCTAVE(3),
  OCTAVE(4),
  OCTAVE(5),
  OCTAVE(6),
  OCTAVE(7),

  /* ninth scale */
  do_8,
};

#undef OCTAVE

struct note_t
{
    uint64_t start_time;
    uint64_t stop_time;
    pitch_t pitch;
    bool is_played;
    uint8_t staff_number;
    std::string id;
};

std::vector<note_t> get_notes(const std::string& filename);

#endif /* NOTES_FILE_EXTRACTOR_HH_ */
