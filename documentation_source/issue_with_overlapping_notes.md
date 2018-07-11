# Fixing overlapping notes

When a key is pressed, it has to be released before one can press it again. While this seems completly obvious, let's not forget
that music sheet notation where made for humans, not computers. And as a consequence, processing music sheet requires some care.

For example, on the following music sheet:

[![simple music sheet with overlaping keys](./issue_with_overlapping_notes_assets/simple_overlapping_notes.svg)](./issue_with_overlapping_notes_assets/simple_overlapping_notes.ly "lilypond source for this simple example")

the first `la` key is meant to be pressed when starting the music, and released at the exact same time the
user should press the `sol` key. In between tough, there is the grace `la` key which should be played after
the "big" `la` and before the `sol` key are pressed. That is, the user should press `la` while it is already pressed. Any
human will understand that to press the `la` the second time, you have to release it first and will
instinctively do it. Therefore the program will look for these situations where a key is meant to be pressed
while it was already pressed. When this happens, the first press key will be changed so that it will be
released when the second one is meant to be pressed.
