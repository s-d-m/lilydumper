# Finding where the cursor position

So far, we got to extract which notes are played when and for how long (except for the caveat with tied notes).
This means based at that point, we can get to play the music, as in send some sound in the computer's speaker
to please someone's ears.

However the goal of the project was also to follow the music sheet. Therefore the program needs to somehow find out
how the notes are laid out, so it can highlight the ones being played currently.

This section explains how we achieved that.
