all: assets
assets:
	lilypond -dbackend=svg -dno-point-and-click simplest.ly ; \
	truncate -s -1 simplest-unnamed-staff.notes ; \
	inkscape --verb=FitCanvasToDrawing --verb=FileSave --verb=FileQuit simplest.svg

clean:
	rm -f simplest.svg \
		simplest-unnamed-staff.notes

.PHONY: clean assets all
