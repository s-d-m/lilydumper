# Finding on which staff a note is

One of the required feature was to be able to play left and right hand separately.
Therefore the software needs to know which part of the music sheet is meant to be
played on the left and which is meant to be played on the right hand.

I believe it is commonly accepted that for piano music sheets, the bottom staff
(that usually starts with a bass clef) and the top staff (usually with a treble clef)
are meant to be played respectively with left and right hand.

Therefore to achieve the goal of discriminating notes from being played left or right, one idea would simply
be to try to use the debug skylines again. Using these, it would be possible to graphically find out on which
staff a note is drawn, and consequently if it is the first or second staff of the system, and therefore if it
should be played with the left or right hand.

See for example the following piece of music.

![Skylines can be used to tell on which staff a note is](./finding_on_which_stave_a_note_is_assets/cross_staves.svg)

However, a different solution was used here.

From the source file, the notes are written in the staff they are. Therefore it is possible starting from a
note to find out in which staff it belongs, only based on the text file. This is faster and simpler than
extracting the same information from the svg files.

Extracting the staff-number is done using the event-listener and the following snippet:

```

#(define (get-staff-number key)
   (let* ((res (assoc key context-to-staff)))
    (if res
      (cdr res) ;; found
      (begin  ;; not found, add it to list
	(let ((res next-staff-num))
	  (set! context-to-staff (cons (cons key res) context-to-staff))
	  (set! next-staff-num (+ 1 next-staff-num))
	  res)))))


#(define (on-note-head engraver grob source-engraver)
   (let* ((context  (ly:translator-context source-engraver))
      ...
	  (root-context (object-address (ly:context-property-where-defined context 'instrumentName)))
	  (staff-number (get-staff-number root-context))
      ...
      )
))

\layout {

  \context {
    \Voice

    \consists #(make-engraver
		(acknowledgers
		 ((note-head-interface engraver grob source-engraver)
		  (on-note-head engraver grob source-engraver))))
  }
}

```

The staff number is then exported in the notes file alongside the id of the note.

The `get-staff-number` function simply assigns an incrementing number to each key it root-context it receives,
starting from 0. That way, the first staff to be encountered will be labeled 0, the next one 1 and so forth.

Each note having the same the same staff number are meant to be played by the same hand.
It then becomes possible to display the keyboard and using different colours, to show
which hands plays what.

This is what the keyboard at the bottom of the [introductory video shows](./intro_assets/lilyplayer-demo.webm "demo"):
  - red coloured keys are pressed by the left hand
  - blue coloured keys are pressed by the right one
