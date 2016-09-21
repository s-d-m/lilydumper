\version "2.16.0"

\pointAndClickOff
#(ly:set-option 'debug-skylines #t)

%%%% Helper functions

%% Now the filename for the note file can be controlled by command line using the following syntax:
%% lilypond -e"(ly:add-option 'note-file-output #f  \"Output for the note file. Default is filename with .notes extension instead of .ly\")" -e"(ly:set-option 'note-file-output \"/path/to/output/note/file\")"

#(define global-variable-filename #f)
#(define (filename-to-output-to)
   (if (not global-variable-filename)
       (let ((option-name (ly:get-option 'note-file-output)))
	 (set! global-variable-filename
	       (if option-name
		   option-name
		   (string-concatenate
		    (list
		     (substring (object->string (command-line))
				;; filename without .ly part
				(+ (string-rindex (object->string (command-line)) #\sp) 2)
				(- (string-length (object->string (command-line))) 5))
		     ".notes"))))))
   global-variable-filename)


%% The filename for the staff number to instrument name table can be controlled by command line using the following syntax:
%% lilypond -e"(ly:add-option 'instrument-name-file-output #f  \"Output for the staff-number-to-instrument-name-table file. Default is filename with .sn2in extension instead of .ly\")" -e"(ly:set-option 'instrument-name-file-output \"/path/to/output/sn2in/file\")"

#(define instr-name-table-filename #f)
#(define (table-filename-to-output-to)
   (if (not instr-name-table-filename)
       (let ((option-name (ly:get-option 'instrument-name-file-output)))
	 (set! instr-name-table-filename
	       (if option-name
		   option-name
		   (string-concatenate
		    (list
		     (substring (object->string (command-line))
				;; filename without .ly part
				(+ (string-rindex (object->string (command-line)) #\sp) 2)
				(- (string-length (object->string (command-line))) 5))
		     ".sn2in"))))))
   instr-name-table-filename)




#(define (moment->frac moment)
    (/ (ly:moment-main-numerator moment)
       (ly:moment-main-denominator moment)))

#(define (moment->frac-nanoseconds moment)
   (exact->inexact
    ;; lilypond moments are stored in seconds unit so multiply by 10^9
    ;; to go to nanoseconds
    (* 1000 1000 1000
       (moment->frac moment))))

#(define (remove.00000000 moment)
   (let* ((str (ly:format "~a" moment))
	  (point-pos (string-index str #\.))
	  (length-to-cut (- (string-length str) point-pos)))
     (string-drop-right str length-to-cut)))


%% default tempo used by lilypond.
%% to find this number, just create a music sheet without setting the tempo
%% and look at which tempo got inserted in the midi file.
#(define time-per-quarter-note (/ 60 60))
#(define last-tempo-moment-change 0)
#(define last-tempo-realtime-change 0)

#(define (moment->real-time moment)
   (let* ((diff-from-last-tempo-change (- (moment->frac moment)
					  last-tempo-moment-change))
	  (nb-quarter-notes (* 4 diff-from-last-tempo-change)))
     (+ last-tempo-realtime-change (* time-per-quarter-note nb-quarter-notes))))

#(define (moment->real-time-nanoseconds moment)
    ;; lilypond moments are stored in econds unit so multiplay by 10^9
    ;; to go to nanoseconds
    (* 1000 1000 1000 (moment->real-time moment)))



#(define was-file-removed? #f)
#(define was-note-option-checked? #f)
#(define should-produce-note-file? #t)

#(define (simple-print-line text)
   (if (not was-note-option-checked?)
       (begin
	 (let ((option (ly:get-option 'disable-notes-output)))
	   (set! should-produce-note-file?
		 (not option)))
	 (set! was-note-option-checked? #t)))
   (if should-produce-note-file?
       (let ((filename (filename-to-output-to)))
	 (if (not was-file-removed?)
	     (begin
	       (if (access? filename F_OK)
		   (delete-file filename))
	       (set! was-file-removed? #t)))

	 (let* ((p (open-file filename "a")))
	   ;; for regtest comparison
	   (display (string-append text "\n") p)
	   (close p)))))


#(define was-table-file-removed? #f)
#(define was-table-option-checked? #f)
#(define should-produce-table-file? #t)

#(define (output-to-table-file text)
   (if (not was-table-option-checked?)
       (begin
	 (let ((option (ly:get-option 'disable-table-output)))
	   (set! should-produce-table-file?
		 (not option)))
	 (set! was-table-option-checked? #t)))
   (if should-produce-table-file?
       (let ((filename (table-filename-to-output-to)))
	 (if (not was-table-file-removed?)
	     (begin
	       (if (access? filename F_OK)
		   (delete-file filename))
	       (set! was-table-file-removed? #t)))

	 (let* ((p (open-file filename "a")))
	   ;; for regtest comparison
	   (display (string-append text "\n") p)
	   (close p)))))


%%% main functions

#(define (format-tempo engraver event)
  (let* ((metronome-count (ly:event-property event 'metronome-count))
	 (tempo-unit (moment->frac (ly:duration-length (ly:event-property
							      event
							      'tempo-unit))))
	 (tempo (/ metronome-count tempo-unit))
	 (context  (ly:translator-context engraver))
	 (moment (ly:context-current-moment context)))
    (set! last-tempo-realtime-change (moment->real-time moment))
    (set! last-tempo-moment-change (moment->frac moment))
    (set! time-per-quarter-note (/ 60 (* metronome-count 4 tempo-unit )))))


#(define (is-tie-articulation? articulation)
  (equal? (ly:prob-property articulation 'name) 'TieEvent))

#(define context-to-staff '())
#(define next-staff-num 0)

#(define (get-staff-number key)
   (let* ((res (assoc key context-to-staff)))
    (if res
      (cdr res) ;; found
      (begin  ;; not found, add it to list
	(let ((res next-staff-num))
	  (set! context-to-staff (cons (cons key res) context-to-staff))
	  (set! next-staff-num (+ 1 next-staff-num))
	  res)))))


#(define (is-grace-note-moment moment)
  (not (zero? (ly:moment-grace-numerator moment))))

#(define (get-start-moment moment)
  (if (is-grace-note-moment moment)
      (ly:make-moment (+ (/ (ly:moment-grace-numerator moment)
			    (ly:moment-grace-denominator moment))
			 (moment->frac moment)))
      ;; else not a grace note, so moment is start moment
      moment))

#(define (get-instrument-name context)
   (if (not context)
       ""
       (let ((instrument-property (ly:context-property context 'instrumentName #f)))
	 (if instrument-property
	     (ly:format "~a" instrument-property)
	     (get-instrument-name (ly:context-parent context))))))


#(define seen-staff-numbers '())
#(define (save-staff-number-instrument-name staff-number context)
   (if (not (member staff-number seen-staff-numbers))
       (begin
	 (output-to-table-file (ly:format "~a ~a"
					  staff-number
					  (get-instrument-name context)))
	 (set! seen-staff-numbers (cons staff-number seen-staff-numbers)))))


#(define (on-note-head engraver grob source-engraver)
   (let* ((context  (ly:translator-context source-engraver))
	  (event (event-cause grob))
	  (pitch  (+ 60 (ly:pitch-semitones (ly:event-property event 'pitch))))
	  (event-duration (ly:event-property event 'duration))
	  (duration-string (ly:duration->string event-duration))
	  (duration (remove.00000000  (moment->frac-nanoseconds (ly:duration-length event-duration))))
	  (origin (ly:input-both-locations
		   (ly:event-property event 'origin)))
	  (layout (ly:grob-layout grob))
	  (music (ly:event-property event 'music-cause))
	  (articulations (ly:music-property music 'articulations))
	  (has-tie-attached (any (lambda (x) (is-tie-articulation? x)) articulations))
	  (root-context (object-address (ly:context-property-where-defined context 'instrumentName)))
	  (staff-number (get-staff-number root-context))
	  (current-bar-number (ly:context-property context 'currentBarNumber))
	  (moment (ly:context-current-moment context))
	  (start-moment (get-start-moment moment))
	  (stop-moment (ly:make-moment
			(+ (moment->frac start-moment)
			   (moment->frac (ly:duration-length event-duration)))))
	  (formated-origin (ly:format "~a:~a:~a:~a"
			     ;; origin is of the form:  (file-name first-line first-column last-line last-column).
                            (ly:find-file (car origin))   ;; find full path name filename
                            (cadr origin)  ;; first line
			    (caddr origin) ;; first column
			    (car (cddddr origin))))
	  ; grace notes have a negative numerator
	  (is-grace-note (is-grace-note-moment moment))
	  (id (ly:format "#origin=~a#pitch=~a#has-tie-attached=~a#staff-number=~a#duration-string=~a#duration=~a#is-grace-note=~a#"
			 formated-origin
			 pitch
			 (if has-tie-attached
			     "yes"
			     "no")
			 staff-number
			 duration-string
			 duration
			 (if is-grace-note
			     "yes"
			     "no")))
	; add the bar number for the id that will go into the svg file
	; the notes file gets its repeats unfolded. as a consequence,
	; the bar numbers won't match those in the svg files (for
	; which the repeats are not unfolded) the end-user will
	; normally trust the bar numbers displayed on the music sheet
	; to be correct. Therefore the svg's bar numbers "take
	; precedence" over the one from the note file.
	(id-with-bar-number (ly:format "#bar-number=~a~a"
				       current-bar-number
				       id)))

	(simple-print-line
	 (format "note start-time: ~d stop-time: ~d id: ~a"
		 (round (moment->real-time-nanoseconds start-moment))
		 (round (moment->real-time-nanoseconds stop-moment))
		 id))
	(save-staff-number-instrument-name staff-number context)
	(ly:grob-set-property! grob 'id id-with-bar-number)
))


%%%% The actual engraver definition: We just install some listeners so we
%%%% are notified about all notes and rests. We don't create any grobs or
%%%% change any settings.

\layout {
  \override NoteHead.stencil = #(lambda (grob)
				  (let* ((note (ly:note-head::print grob))
					 (former-id (ly:grob-property-data grob 'id))
					 (x-interval (ly:stencil-extent note X))
					 (x-width (interval-length x-interval))
					 (y-interval (ly:stencil-extent note Y))
					 (y-height (interval-length y-interval))
					 (new-values (format "#x-width=~1,4f#y-height=~1,4f" x-width y-height))
					 (new-id (string-append new-values former-id)))
				    (ly:grob-set-property! grob 'id new-id)
				    note))

  \context {
    \Voice
    \consists #(make-engraver
		(listeners
		 (tempo-change-event . format-tempo)))

    \consists #(make-engraver
		(acknowledgers
		 ((note-head-interface engraver grob source-engraver)
		  (on-note-head engraver grob source-engraver))))
  }
}
