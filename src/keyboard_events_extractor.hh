#ifndef KEYBOARD_EVENTS_EXTRACTOR_HH_
#define KEYBOARD_EVENTS_EXTRACTOR_HH_

#include <vector>
#include "notes_file_extractor.hh"

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
};

struct key_event
{
    uint64_t time; // the time the event occurs during the sond
    key_data data;
};


std::vector<key_event> get_key_events(const std::vector<note_t>& notes);


#endif /* KEYBOARD_EVENTS_EXTRACTOR_HH_ */
