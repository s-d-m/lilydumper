# Issue: tied notes

The issue with tied notes it that they are ... tied.
Let's look at the following example to understand what the issue actually is:

[![simple music sheet with tied notes](./issue_with_tied_notes_assets/simplest_with_tied_notes.svg)](./issue_with_tied_notes_assets/simplest_with_tied_notes.ly "lilypond source for this simple example with tied notes")

This music sheet means that the `la` key must be pressed only once, and for a total of 1.2s (two quarter time)
However, when simply using the default event listener, the output we get is as follow:
```
{{#include ./issue_with_tied_notes_assets/simplest_with_tied_notes-unnamed-staff.notes}}
```
The event listener generates lines per ... event, and on the music sheet there are two notes, hence two lines.
Instead of saying there should only be one key pressed, it says the key should be pressed twice, which is wrong.

My first approach to tackle this issue was to use the midi file. [Lilypond](http://lilypond.org) can generate
a midi file alongside the music sheet. As a rough estimate approximation, a midi file is akin to an mp3 file
but for instrument music only.  Since lilypond generates midi file that correctly describes how the music is played, it
would be possible to know if a key is played longer than what is described in the events file, and thus if it
is tied to the next key with the same pitch.  This lead me to develop
[pianoterm](https://github.com/s-d-m/pianoterm). However some issues with this approach quickly appeared. When
playing triolet for example, there is a slight difference in the timings outputed in the events file and
the one in the midi files due to some rounding in the midi file. This could be easily fixed by correlating the
timings, however this approach looked a bit too britle hence I looked for something else.

On the generated events above, we can see there is a `tie` event. We could use this as it means the previous
note has a tie attached, but sadly this is not precise enough. For example, the following two equivalents
music sheets provides the following two different events:

| music sheet | lilypond source | generated events |
|-------------|-----------------|------------------|
| [![tied_notes_chord_1](./issue_with_tied_notes_assets/tied_notes_chord_1.svg)](./issue_with_tied_notes_assets/tied_notes_chord_1.ly  "lilypond source for this simple example with tied notes") | <la' sol'>4~ <la' sol'> |  <object type="text" data="./issue_with_tied_notes_assets/tied_notes_chord_1-unnamed-staff.notes"></object> |
| [![tied_notes_chord_2](./issue_with_tied_notes_assets/tied_notes_chord_2.svg)](./issue_with_tied_notes_assets/tied_notes_chord_2.ly  "lilypond source for this simple example with tied notes") | <sol' la'>4~ <sol' la'> |  <object type="text" data="./issue_with_tied_notes_assets/tied_notes_chord_2-unnamed-staff.notes"></object> |

On the first first music sheet, looking at the generated events only, it looks like the tie applies to the `sol` key,
whereas on the second one it looks like it applies to the `la` key. Truth is, both the `sol` and `la` key have a tie
attached.

For an even more contrived example, the two following distinct music sheets give the same tie events (as
seen per the event-listener)

| music sheet | Source | generated events |
|------------|---------|--------|
| [![tied notes_with_same_tie_events1](./issue_with_tied_notes_assets/tied_notes_with_same_tie_events1.svg)](./issue_with_tied_notes_assets/tied_notes_with_same_tie_events1.ly "lilypond source for this simple example with tied notes") | <sol' la'~ mi''>4~  <sol' la' mi''>4 | <object type="text" data="./issue_with_tied_notes_assets/tied_notes_with_same_tie_events1-unnamed-staff.notes"></object> |
| [![tied notes_with_same_tie_events2](./issue_with_tied_notes_assets/tied_notes_with_same_tie_events2.svg)](./issue_with_tied_notes_assets/tied_notes_with_same_tie_events2.ly "lilypond source for this simple example with tied notes") | <sol' la' mi''>4~ la' | <object type="text" data="./issue_with_tied_notes_assets/tied_notes_with_same_tie_events2-unnamed-staff.notes"></object> |


One solution to overcome this issue would be to get on each line corresponding to a note, something saying if
yes or no there is a tie attached to that note.

Since Lilypond provides the option to let the user include file for global settings, I used it to extract more data
from the event-listener than the default one.

That is, I created a file named `event-listener.scm` with [this content](./issue_with_tied_notes_assets/event-listener.scm.for_tied_notes_only) and then called lilypond using `lilypond
-dinclude-settings=event-listener.scm <music-sheet.ly>`.

The following table shows the new event-listener's output:


| music sheet | source |  generated events |
|------------|---------| --------|
| [![tied_notes_chord_1](./issue_with_tied_notes_assets/tied_notes_chord_1.svg)](./issue_with_tied_notes_assets/tied_notes_chord_1.ly  "lilypond source for this simple example with tied notes") | <la' sol'>4~ <la' sol'> |  <object type="text" data="./issue_with_tied_notes_assets/tied_notes_chord_1_tie_event_on_note"></object> |
| [![tied_notes_chord_2](./issue_with_tied_notes_assets/tied_notes_chord_2.svg)](./issue_with_tied_notes_assets/tied_notes_chord_2.ly  "lilypond source for this simple example with tied notes") | <sol' la'>4~ <sol' la'> |  <object type="text" data="./issue_with_tied_notes_assets/tied_notes_chord_2_tie_event_on_note"></object> |

Sadly, on these generated events, it says no notes have a tie attached. On a positive side we can say that at least
we got some consistency here. Two different inputs which are logically equivalent now generates the same output,

One way to workaround the issue, is to rewrite the music sheet to explicitly say that each note on the chord
has a tie attached instead of saying the whole chord has a tie attached. Below show the difference in writing
the input, and the generated output.


| music sheet | source |  generated events |
|------------|---------| --------|
| [![separately_tied_notes_chord](./issue_with_tied_notes_assets/separately_tied_notes_chord.svg)](./issue_with_tied_notes_assets/separately_tied_notes_chord.ly  "lilypond source for this simple example with tied notes") | <la'~ sol'~>4 <la' sol'> |  <object type="text" data="./issue_with_tied_notes_assets/separately_tied_notes_chord_tie_event_on_note"></object> |
| [![separately_tied_notes_chord2](./issue_with_tied_notes_assets/separately_tied_notes_chord2.svg)](./issue_with_tied_notes_assets/separately_tied_notes_chord2.ly  "lilypond source for this simple example with tied notes") | <sol' la'~ mi''>4 la' |  <object type="text" data="./issue_with_tied_notes_assets/separately_tied_notes_chord2_tie_event_on_note"></object> |
| [![separately_tied_notes_chord3](./issue_with_tied_notes_assets/separately_tied_notes_chord3.svg)](./issue_with_tied_notes_assets/separately_tied_notes_chord3.ly  "lilypond source for this simple example with tied notes") | <sol'~ la'~ mi''~>4   <sol' la' mi''>4 |  <object type="text" data="./issue_with_tied_notes_assets/separately_tied_notes_chord3_tie_event_on_note"></object> |


Now we properly get for each note if there is a tie attached to it. This solution is not satisfactory
as it requires modifying by hand the input music sheet, but sadly I could not find anything better yet.
