\version "2.18.2"
\language "italiano"

\header {
  % Remove default LilyPond tagline
  tagline = ##f
}

global = {
  \key do \major
  \time 4/4
  \tempo 4=100
}

right = \relative do'' {
  \global
  do, re mi

}

left = \relative do' {
  \global
  do, re mi
}

\score {
  \new PianoStaff \with {
    %instrumentName = "Piano"
  } <<
    \new Staff = "right" \with {
      instrumentName = "Pit"
    } \right
    \new Staff = "left" \with {
      instrumentName = "Herr"
    } { \clef bass \left }
  >>
  \layout { }
}
