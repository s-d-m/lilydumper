\version "2.18.2"
\language "italiano"

\include "event-listener.ly"

%% #(set! paper-alist (cons '("my size" . (cons (* 15 in) (* 3 in))) paper-alist))


\paper {
  indent = 0\mm
  line-width = 110\mm
  oddHeaderMarkup = ""
  evenHeaderMarkup = ""
  oddFooterMarkup = ""
  evenFooterMarkup = ""
%%   #(set-paper-size "my size")

}

\header {
    % Remove default LilyPond tagline
  tagline = ##f
}

global = {
  \key do \major
  \numericTimeSignature
  \time 4/4
  \tempo 4=100
}

\score {
  {
    \global

      la' \grace{la'} sol'
  }
}
