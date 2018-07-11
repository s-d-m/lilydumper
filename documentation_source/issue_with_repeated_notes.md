# Separating repeated notes

The following music sheet:

[![one note repeated twice](./issue_with_repeated_notes_assets/simple_repeat.svg)](./issue_with_repeated_notes_assets/simple_repeat.ly "lilypond source for this simple example")


means that:

1. at the very beginning of the song, that is at \\( t=0s \\), a pianist must press the `la` key.
1. at \\( t=0.6s \\), he must release the `la` key and immediately press it again
1. at \\( t=1.2s \\), he must release the `la` key.

Releasing a key and pressing it again _at the exact same time_ is something that is literraly impossible to do
for a human being. However, for a computer this lead to the situation where a sound is emitted but it doesn't look
like the key was pressed again.

Let's look at the following video, and let's pay extra attention to the `la` key displayed on the keyboard at the bottom.
It looks like it is pressed at the beginning and released at the end only, while in the middle of the video we can
distinctly hear the `la` note being played again.

[![The key on the keyboard looks like it is pressed only once](./issue_with_repeated_notes_assets/fake_screenshot_repeated_notes.png)](./issue_with_repeated_notes_assets/repeated_notes_without_spacing.webm "The key on the keyboard looks like it is pressed only once")

When looking at the keyboard only, it looks like the `la` key is played once and held as if it was a white note, not a quarter one.

To overcome this issue, the program applies a similar strategies as the one used to fix "overlapping" notes. It will detect when
a note is said to be released and pressed at the exact same time, and it will then introduce a small temporal gap. This mimick
human's behaviour. The first note will be released slightly earlier. Its duration will be shorter by either 1/4 of its duration,
or 75 milli seconds, whichever is smaller. These values where chosen completely arbitrarily though. The result is the following
video where one can clearly spot when is second `la` played only by looking at the keyboard.

[![One can see the key being pressed again](./issue_with_repeated_notes_assets/fake_screenshot_repeated_notes.png)](./issue_with_repeated_notes_assets/repeated_notes_with_small_spacing.webm "Adding a small spacing let one see the key being pressed again") |
