# Lilyplayer

Lilyplayer plays piano music sheets.

For a very quick overview of what it does, watch the following video:

[![Demo](./intro_assets/lilyplayer-fake-video-screenshot.png)](./intro_assets/lilyplayer-demo.webm "demo")

The initial goal of the project was broader than simply that. At first, it was meant to be a full-featured
piano learning app.

For this project, I wanted to have the following features:

- Real music sheets

	Instead of boxes coming down the screen like [synthesia](http://synthesiagame.com), the application must
	display a real music sheet.  The goal being to help learning piano, not to be a competitive game.

- Users must be able to use their own music sheets

	The application must not prevent the user from using any music sheet he wants. If the user bought a copy
	of some music sheet, he should be able to use it in the app. Therefore, the process of using a personal music
	sheet must be documented.

- Only one quality tolerated for the music sheets, the very best one.

	No one like to read music sheets scanned in so poor quality they appear pixelated on the screen. Also, the
	music sheet must look great on all screens, regarless of their dpi.

- Practice only a subpart of the music sheet

    No one learns a full music sheet right away. People practice on smaller parts at a time, and the app
	should let people do just that.

- Ability to play left and right hand separately

	It is often useful to practice the left and right hand part of a music separately. The app should not go
	into people's way of learning.

Based on these requirements, it is no surprise that I decided to base my solution on top of
[lilypond](http://lilypond.org). In their own words:

```quote
LilyPond is a music engraving program, devoted to producing the highest-quality sheet music
possible. It brings the aesthetics of traditionally engraved music to computer printouts.

```

Lilypond's goal is to engrave music sheets. It takes a music description in a text format as input and
generates a beautiful pdf as output.  For this project this is simply not enough. To play music sheet, as
showcased in the video above, more information needs to be retrieved.

The rest of the document describes the challenges that appeared during the project, and how I
overtook them (or not).
