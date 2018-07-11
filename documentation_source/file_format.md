# File format

The whole process described up to here takes time. To give you an idea, it takes about 20 seconds to analyse
the music sheet "Für Elise" from Beethoven. Surely, having to wait 20 seconds every time one wants to listen
to that song is simply not acceptable. Also, since this computation gives always the same result, one can
simply compute the whole thing once, and then save the result somewhere to later reuse.

This chapter describe the file format used, so that applications reading these files can be built.
The program that creates this file is named lilydumper, and the one that plays it is named lilyplayer.

The file format is made of the following:
 - a header
 - the mapping staff number, instrument name
 - a list of events
 - the svg files

And nothing else. There should be no data after the last svg file, and reader program should not accept files
with more data at the end.

## The header

The first four bytes of the file are a magic number and must match `LPYP` in ascii. That is a lilydumper file
must start by `0x4c 0x50 0x59 0x50`.
The next byte is a version number. It was meant to provide backward compatibility in case new of new features
that would end up in the file format. For now it is basically `0x00`. Any other value is unknown.

## Mapping staff number, instrument name

Immediately follwoing the header, comes the instrument names mapping as described in [its own
chapter](./finding_the_staff_instrument_name.md).  First there is one byte that tells how many instrument
names there is to come. This will likely be `0x02` because a piano sheet music has most of the times two
staves. Then comes the names of each staff written as utf-8 null-terminated string. The first string is the
name of the staff number 0, the second one is for the staff number 1 and so forth.

An example here could be: `0x02 0x50 0x69 0x61 0x6e 0x6f 0x00 0x50 0x69 0x610x6e 0x6f 0x00`. The first `0x02`
means there are two names coming which are `Piano` (0x50 0x69 0x61 0x6e 0x6f 0x00) and `Piano` again.

## The events

Events occuring at the same time are grouped together, forming a group of events.  The events section of the
file starts by the number of group of events. This number is stored in 8 bytes as a big endian value. Then there
are that number of group of events appearing right after.

Group of events are written out sequentially, in chronological order.

Each group of events is written as follow:

1. time of occurrence in nanoseconds from the start of the song (8bytes big endian)
1. number of events occurring at that time (1byte)
1. followed by each event occuring at that time.

There are five different types of event so far. Each of them has a specified output format. The way they are
written is described below:

1. id of event (1byte)
1. event-specific data

Below is the table of event-id

| event         | value |
|----------------|-------|
| press key      |     0 |
| release key    |     1 |
| set bar number |     2 |
| set cursor     |     3 |
| set svg file   |     4 |


The event-specific data is:

**for press key event**
1. pitch to play (1 byte)
1. staff number  (1 byte)  (staff number on which the note appears on the music sheet)

**for release key event**
1. pitch to release (one byte)

**for set bar number**
1. the new bar/measure number (2 bytes, Big endian) (this is the current measure in the music sheet)

**for set cursor**
1. the left coordinate of the cursor box (4bytes BE)
2. the right coordinate (4bytes BE)
3. the top coordinate (4bytes BE)
4. the bottom coordinate (4bytes BE)

These coordinate are stored in the file as integer but must be divided by 10.000 to use. For example, a set cursor
event with the following values in the file


| postition | value |
|------|-----|
| left | 520608 |
| right | 750000 |
| top | 1234567 |
| bottom | 2345678 |


means the reader program must use create the following rectange as svg:
```svg
<rect x="52.0608" y="123.4567" width="22.9392" height="111.1111/>
```

Also note that point(0.0) means top left corner and point(5, 10) is 5 units on the right and 10 below.
Therefore, the right value of the cursor box is always bigger than the left value. Same goes for the
bottom value which is also always bigger than the top value.

**for set svg file**

1. the number of the svg file to be displayed now (2bytes BE)

Note that the svg files are 0-indexed. A value of 0 means the first page. 1 means the second page and so on.


## The svgs section

This section is made of:

1. the number of svg files the music contains. (two bytes, big endian)
1. For each of the svg file:
  1. four bytes (big endian) telling the size in bytes of the svg file.
  1. the content of that svg file.

The svg files are stored in the list in order of appearance. Therefore the first one if the file
is the first page (page number zero for the turn-page event).

> This chapter could be summed up as "If I had to do it again, I would simply use Cap'n Proto".
