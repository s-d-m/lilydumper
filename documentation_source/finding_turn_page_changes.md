# Finding turn page events

When playing the music it is important to know when to turn the page, otherwise the cursor will be shown at the
right place but on the wrong page. One solution here could be to always say on which page should a cursor be printed.
However that would be wasteful as the page doesn't change often. Therefore instead of always setting the page to the
current one, the page will only be selected when there is a turn page event.

Detecting a turn page is simple. When playing the music, if two consecutive cursor are on two different pages, there
must be a turn-page event inserted. The new page is the one of the second cursor. Note that the page of the second
cursor is not necessarily the one coming after the page of the first cursor. For example, in case of a repeated part
that starts at the bottom of a page and finishes at the top of the next one. There would be a turn page event going
from page 1 to 2, and when hitting the repeat bar the first time, it will go back to page 1.
