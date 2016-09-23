#pragma once

#include <experimental/filesystem>
#include <string>
#include <vector>

std::string get_value_from_field(const std::string& id_str, const char* const field);

namespace fs = std::experimental::filesystem;

fs::path get_temp_dir();


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


struct key_data
{
    enum type : bool
    {
      pressed,
      released,
    };

    uint8_t  pitch; // the key that is pressed or released
    type     ev_type; // was the key pressed or released?
    uint8_t  staff_number; // used to color keys

    bool operator==(const key_data& other) const
    {
      return (pitch == other.pitch) and
	     (ev_type == other.ev_type) and
	     (staff_number == other.staff_number);
    }
};

struct key_event
{
    uint64_t time; // the time the event occurs during the sond
    key_data data;

    bool operator==(const key_event& other) const
    {
      return (time == other.time) and (data == other.data);
    }
};


struct chord_t
{
    chord_t()
      : notes()
    {
    }

    // a chord are just notes played at the same time
    std::vector<note_t> notes;
};

void debug_dump(const std::vector<key_event>& song, const char* const out_filename);
void debug_dump(const std::vector<note_t>& song, const char* const out_filename);
void debug_dump(const std::vector<chord_t>& chord, const char* const out_filename);
void debug_dump(const std::vector<std::string>& strings, const char* const out_filename);
