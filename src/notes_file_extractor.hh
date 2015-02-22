#ifndef NOTES_FILE_EXTRACTOR_HH_
#define NOTES_FILE_EXTRACTOR_HH_

#include <vector>

#define octave(X) \
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

  octave(1),
  octave(2),
  octave(3),
  octave(4),
  octave(5),
  octave(6),
  octave(7),

  /* ninth scale */
  do_8,
};

#undef octave

struct note_t
{
    uint64_t start_time;
    uint64_t stop_time;
    uint16_t bar_number;
    pitch_t pitch;
    bool is_played;
    std::string id;
};

std::vector<note_t> get_notes(const std::string& filename);

#endif /* NOTES_FILE_EXTRACTOR_HH_ */
