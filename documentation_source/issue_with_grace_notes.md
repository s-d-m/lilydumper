# Issue: grace notes

```quote
Grace notes are musical ornaments, printed in a smaller font,
that take up no additional logical time in a measure.
```

The issue with grace notes is that as the quote above says, says take _no_ additional logical time in a measure.


Let's take an example. On the following images

[![simple grace note](./issue_with_grace_notes_assets/simple_grace_notes.svg)](./issue_with_grace_notes_assets/simple_grace_notes.ly "lilypond source for this simple example with tied notes")

This means that:

1. at the very beginning of the song, that is at \\( t=0s \\), a pianist must press the `la` key (aka `a`).
1. at \\( t=0.6s \\), he must release the `la` key and immediately press it again
1. at \\( t=1.2s \\), he must release the `la` key.
1. and somewhen after pressing the `la` key the first and second, he must quickly press and release the `sol` key.

However the pressing and release time for the grace key (here `sol`) are not well defined.

Now let's look at the generated events:

| music sheet | lilypond source | generated events |
|-------------|-----------------|------------------|
| [![simple grace note](./issue_with_grace_notes_assets/simple_grace_notes.svg)](./issue_with_grace_notes_assets/simple_grace_notes.ly "lilypond source for this simple example with tied notes") |     la' \grace{sol'} la' |  <object type="text" data="issue_with_grace_notes_assets/simple_grace_notes-unnamed-staff.notes"></object> |

Here we can easily see that the grace note has a starting time set to `0.25000000-0.25000000`. This means the grace note must have finished by the end of the first quarter note.

On the slightly more complicated sheet below, which contains two successive grace notes,

| music sheet | lilypond source | generated events |
|-------------|-----------------|------------------|
| [![two grace notes](./issue_with_grace_notes_assets/two_grace_notes.svg)](./issue_with_grace_notes_assets/two_grace_notes.ly "lilypond source for this simple example with tied notes") |     la' \grace{sol' sol'} la' |  <object type="text" data="issue_with_grace_notes_assets/two_grace_notes-unnamed-staff.notes"></object> |

we can see they have a starting time of respectively `0.25000000-0.50000000` and `0.25000000-0.25000000`.
The part before the dash is the time they have to finish. The part after can be used to know order them.

To decide when to play a grace note, and for how long, the algorithm used is quite simple, it will:

1. Try to fit all successive grace note fairly distributed between the former and next "normal" notes, and use
   all the time in between these two notes
1. prevent grace note from being played longer than (29 / 128) * (60 / 100) seconds to match the midi file
   produced by lylipond. If the former and next normal note are separated by a longer duration such that this
   maximum duration limit is hit, then grace notes are played at the end of the interval. That is, they are
   played right before the following next normal note.
