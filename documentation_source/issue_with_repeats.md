# Repeats

The idea of using the event listeners to extract when notes are played shows its limits quite well in the handling of repeats.
The event listeners reports events as it sees them in the lilypond's input file. For example on the following music sheet

[![simple music sheet with repeat](./issue_with_repeats_assets/simple_repeat.svg)](./issue_with_repeats_assets/simple_repeat.ly "lilypond source for this simple example with tied notes")

the `la` should be played twice. However the generated events are

```
{{#include ./issue_with_repeats_assets/simple_repeat-unnamed-staff.notes}}
```

when using the default event-listener provided by lilypond. That is the exact same events as generated

[![simple music sheet without any repeat](./issue_with_repeats_assets/no_repeat.svg)](./issue_with_repeats_assets/no_repeat.ly "lilypond source for this simple example with tied notes")

This issue here is due to the fact that the default event-listener doesn't pay attention to repeat events.
I tried modifying it in several ways so that it would also report on repeat events.
However I could not get what was repeated which is quite important to say the least.

if we have the following music sheet,


```
\repeat volta 2 { la' sol' }
```

We need to extract that this is equivalent to `la' sol' la' sol'`.
With my several attempts based only on the event-listener, I was getting the following events.:

```
t=0                  play la'   duration quarter-time
t=quarter-time       play sol'  duration quarter-time
t=2 * quarter-time   display-repeat-bar
```

This is not good enough, as the same output was produced for this different music:

```
la' \repeat volta 2 { sol' }
```
which is equivalent to `la' sol' sol'`.

Here the solution was to unfold all repeats first.

Lilypond handle the following kind of repeats as described in its [documentation](http://lilypond.org/doc/v2.19/Documentation/notation/repeats):
- volta
- unfold
- percent
- tremolo

Below is a table showing the effect of each repeat type on the event-listener output

| music sheet | source |  generated events |
|------------|---------| --------|
| [![volta repeat](./issue_with_repeats_assets/volta_repeat.svg)](./issue_with_repeats_assets/volta_repeat.svg "lilypond source for this simple example") | \repeat volta 2 { la' } | <object type="text" data="./issue_with_repeats_assets/volta_repeat-unnamed-staff.notes"></object> |
| [![unfold repeat](./issue_with_repeats_assets/unfold_repeat.svg)](./issue_with_repeats_assets/unfold_repeat.svg "lilypond source for this simple example") | \repeat unfold 2 { la' } | <object type="text" data="./issue_with_repeats_assets/unfold_repeat-unnamed-staff.notes"></object> |
| [![percent repeat](./issue_with_repeats_assets/percent_repeat.svg)](./issue_with_repeats_assets/percent_repeat.svg "lilypond source for this simple example") | \repeat percent 2 { la' } | <object type="text" data="./issue_with_repeats_assets/percent_repeat-unnamed-staff.notes"></object> |
| [![tremolo repeat](./issue_with_repeats_assets/tremolo_repeat.svg)](./issue_with_repeats_assets/tremolo_repeat.svg "lilypond source for this simple example") | \repeat tremolo 2 { la' } | <object type="text" data="./issue_with_repeats_assets/tremolo_repeat-unnamed-staff.notes"></object> |

`Percent` and `tremolo` repeats are still something I can't really understand even after reading [the documentation on short repeats](http://lilypond.org/doc/v2.19/Documentation/notation/short-repeats).
Therefore I just ignore them. However, when focusing on `volta` and `unfold` repeat, we see on the generated events that unfolded repeats
produce the correct events about notes being played and when. Therefore when encoutering a `volta` repeat, one can simply change it to
an `unfold`ed one to get the right music events. This leads to another issue though. It changes the generated output. One goal of the
project was to work on real music sheets, just like the ones a pianist would use. Not a simplified equivalent one. This issue will
be treated later on, in the part about [matching a note to where it appears on the music sheet](./matching_a_note_to_where_it_appears.md).

For now, to solve the current issue due to repeats, let's just replace all `volta` repeats by `unfold` repeats.
There are several ways to do so. Since lilypond's input file are simply text files, one way would be to
programmatically edit the text to replace `volta` by `unfold` everywhere it appears, and then start processing
the music sheet. Another way would be to modify lilypond so that every time it encounters a `volta` repeat it
would treat it just like an `unfold` one. I went for the second solution as it is the most stable one as
modifying the source would lead to other issues later on.

Reading lilypond's source code, I found [the place where it decide how to treat a repeat](https://git.savannah.gnu.org/gitweb/?p=lilypond.git;a=blob;f=scm/music-functions.scm;h=7d70c9bbd5652b75a0ac71b27f07bda42e399fe4;hb=a358ea26328939acdcfb0f08f307bb1c3b076915#l345).
simply changing `VoltaRepeatedMusic` by `UnfoldedRepeatedMusic`. Modifying lilypond implies recompiling
it, which also implies having the right compile-time dependencies and other inconveniences. Fortunately,
the file requiring the modification is actually interpreted, hence simply modifying and re-running lilypond
is enough. And to avoid file-system related issues, the modification is made on the fly when lilypond opens
that file. This is done by using a library that overwrites the system's `open` and `fopen` function and is set
via `LD_PRELOAD` environment variable.
