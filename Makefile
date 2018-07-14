all: lilydumper

scan-build lilydumper:
	${MAKE} -C ./src "$@"

doc book documentation:
	${MAKE} -C "./documentation_source" assets
	mdbook build

clean:
	${MAKE} -C ./src "$@"
	rm -rf docs

appimage: lilydumper
	./make-appimage.sh

.PHONY: all scan-build lilydumper doc book documentation clean appimage
