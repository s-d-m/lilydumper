all: assets
assets: images events_with_tie_event_on_note_line


images:
	for sheet in *.ly ; do \
	  rm $${sheet%%.ly}-unnamed-staff.notes ; \
	  lilypond -dbackend=svg -dno-point-and-click $$sheet ; \
	  truncate -s -1 $${sheet%%.ly}-unnamed-staff.notes ; \
	done
	for svg in *.svg ; do \
	  inkscape --verb=FitCanvasToDrawing --verb=FileSave --verb=FileQuit $$svg ; \
	done


events_with_tie_event_on_note_line:
	for file in tied_notes_chord_1.ly \
		    tied_notes_chord_2.ly \
		    separately_tied_notes_chord.ly  \
		    separately_tied_notes_chord2.ly  \
		    separately_tied_notes_chord3.ly ; \
	do \
	  rm -f $${file%%.ly}_tie_event_on_note ; \
	  grep -v 'event-listener.ly' $${file} > $${file%%.ly}-without-event-listener-in-it ; \
	  lilypond -dbackend=null -dinclude-settings=event-listener.scm.for_tied_notes_only \
		   --evaluate="(ly:add-option 'note-file-output #f \"\")" \
		   --evaluate="(ly:set-option 'note-file-output \""$${file%%.ly}_tie_event_on_note"\")" \
		   $${file%%.ly}-without-event-listener-in-it ; \
	  truncate -s -1 $${file%%.ly}_tie_event_on_note ; \
	done


clean:
	rm -f  *.notes *.svg *.pdf *.sn2in *~ *.eps *.tex *.texi *.count \
		events_for_tied_notes1 \
		events_for_tied_notes2 \
		tied_notes_chord_1_tie_event_on_note_line \
		tied_notes_chord_2_tie_event_on_note_line \
		tied_notes_chord_1_tie_event_on_note \
		tied_notes_chord_2_tie_event_on_note \
		separately_tied_notes_chord_tie_event_on_note \
		separately_tied_notes_chord2_tie_event_on_note \
		separately_tied_notes_chord3_tie_event_on_note \
		separately_tied_notes_chord-without-event-listener-in-it \
		separately_tied_notes_chord2-without-event-listener-in-it \
		separately_tied_notes_chord3-without-event-listener-in-it \
		tied_notes_chord_1-without-event-listener-in-it \
		tied_notes_chord_2-without-event-listener-in-it

.PHONY: clean assets images events_with_tie_event_on_note_line all
