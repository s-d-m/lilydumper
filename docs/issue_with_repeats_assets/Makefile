all: assets
assets: images

images:
	for sheet in *.ly ; do \
	  rm $${sheet%%.ly}-unnamed-staff.notes ; \
	  lilypond -dbackend=svg -dno-point-and-click $$sheet ; \
	  truncate -s -1 $${sheet%%.ly}-unnamed-staff.notes ; \
	done
	for svg in *.svg ; do \
	  inkscape --verb=FitCanvasToDrawing --verb=FileSave --verb=FileQuit $$svg ; \
	done


clean:
	rm -f *.svg *-unnamed-staff.notes


.PHONY: all assets images clean
