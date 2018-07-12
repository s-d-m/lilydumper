Lilydumper
==========

This program takes a piano music sheet as lilypond file as input, and generates a new file suitable to use with lilyplayer.
If you just want to use some premade music sheets, you can grab some [here](https://github.com/s-d-m/precompiled_music_sheets_for_lilyplayer).

Documentation is available at [https://s-d-m.github.io/lilydumper/](https://s-d-m.github.io/lilydumper/)

Build dependencies
----------------

`Lilydumper` uses pugixml. A C++17 compiler is also required. On debian you can install them using:

	sudo apt-get install libpugixml-dev g++-8


Compiling instructions
-------------------

Once all the dependencies have been installed, you can simply compile `lilydumper` by entering:

	make

This will generate the `lilydumper` binary in `./bin`


How to use:
------------

Lilydumper is based on lilypond, therefore it has to be installed too. This can be achieved using:

	sudo apt-get install lilypond

Then simply run `lilydumper -o <path_to_output_file> -i <source_lilypond_file>`

If all goes well, a file will be produced suitable for use by `lilyplayer`


Bugs & questions
--------------

Report bugs and questions to da.mota.sam@gmail.com (I trust the anti spam filter)
