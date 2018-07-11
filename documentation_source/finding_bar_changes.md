# Finding bar changes events

One of the required feature for the program was to be able to play only one part of the music, ... because
people (at least beginners) learn a song one part at a time. A natural way to break a song into pieces is
at measure boundary. Therefore the lilyplayer at to provide a way to play the music from some measure up to
some other. this was visible in the [introductory video](./intro_assets/lilyplayer-demo.webm) at second 21.

To find out when a measure starts and ends, I used the `currentBarNumber` from lilypond on the event-listener.
On the event-listener, when getting notified of a note head, we retrieve the bar number and set it as part of
its id.

The code is as follow:
```

#(define (on-note-head engraver grob source-engraver)
   (let* ((context  (ly:translator-context source-engraver))

      ... code to get the event ,,,

	  (current-bar-number (ly:context-property context 'currentBarNumber))
	  (id ... former value t=with origin, start time, duration etc.))

      (id-with-bar-number (ly:format "#bar-number=~a~a"
				          current-bar-number
				          id)))

 	 (ly:grob-set-property! grob 'id id-with-bar-number)
))

\layout {
  \override NoteHead.stencil = #(lambda (grob)
				  (let* ((note (ly:note-head::print grob))
					 (former-id (ly:grob-property-data grob 'id)))
					 ... code to add the width and height of the note bounding box ...
				     (ly:grob-set-property! grob 'id former-id)
				    note))

  \context {
    \Voice

	...

    \consists #(make-engraver
		(acknowledgers
		 ((note-head-interface engraver grob source-engraver)
		  (on-note-head engraver grob source-engraver))))
  }
}
```

Now, this bar number will only be written as part of the note id when running lilypond to get the svg
file. Since we unfold the repeats to extract when notes are played, using the bar number as reported with
unfolded repeat will lead to different values than what the user would see on the music sheet which contains
repeats.

That way, it is possible to know to which measure belongs a note. Detecting measure number change is as simple
as checking two consecutively played notes and checking whether their measure number are the same or note.

To then find out when a measure `x` starts, one just has to find the first played note with bar number `x`.

Providing the user with the ability to play only from measure say 4 to 10 requires a bit more thought, in
case there are repeats. For example, if measure 7 ends with a repeat returning to measure 2 and that this
repeat is meant to be hit twice before moving to measure 8, what should the software play?
  - measure 4 to 8 then 3 to 8 then 3 to 8 again then 9 and 10?
  - measure 4 to 8 then 3 to 8 then 9 and 10?
  - measure 4 to 8 then 9 and 10?

Here the choice should probably be left to the user. Sadly lilyplayer doesn't let the user choose and decides
to play the longest piece.
