# Extracting when a note is played, and for how long

One of the challenge was to find out when a note is played, and for how long.

For example, on the following simple piece of music,

[![simple music sheet](./finding_when_notes_are_played_assets/simplest.svg)](./finding_when_notes_are_played_assets/simplest.ly "lilypond source for this simple example")

the program must understand that:

1. at the very beginning of the song, that is at \\( t=0s \\), a pianist must press the `la` key (aka `a`).
1. at \\( t=0.6s \\), a pianist must release the `la` key and simultaneously push the `sol` key down (`g`)
1. at \\( t=1.2s \\), a pianist must release the `sol` key.



To extract these pieces of information I used two features provided by [Lilypond](http://lilypond.org): the
`include-settings` parameter, and the event listener.

`include-settings` let the user include a file to set some global settings. That file is included before the
score is processed.  [The event
listener](http://lilypond.org/doc/v2.18/Documentation/notation/saving-music-events-to-a-file) saves music
events encountered during the music processing.

For this simple music sheet
[![simple music sheet](./finding_notes_played_general_idea_assets/simplest.svg)](./finding_notes_played_general_idea_assets/simplest.ly "lilypond source for this simple example"), the generated music events are:
```
{{#include ./finding_notes_played_general_idea_assets/simplest-unnamed-staff.notes}}
```
That is with the default event-listener. Lilypond provides a way for the user to change that so more informations can be extracted.

Out of this, we get that the tempo is 400. This 400 is almost the tempo we wrote. This 400 comes from the fact
that we defined a tempo based on _quarter_ note.

The next line says that at \\( t = 0 \\) the key 69 (computer code for `la`) is played for a quarter note.
And a quarter note after the beginning, the note 67 (key code for `sol`) is also played for a quarter note.

There are some basic maths involved to get from these tempo and quarter timing into time a computer can use to
understand when to start playing a note, and when to stop.

One important thing to keep in mind though, is that the tempo can change during the music. These new tempos
need to be taken into account when computing the time a note is played released.
