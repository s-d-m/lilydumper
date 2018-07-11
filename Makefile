all: lilydumper documentation

scan-build lilydumper:
	${MAKE} -C ./src "$@"

doc book documentation:
	${MAKE} -C "./documentation_source" assets
	mdbook build

clean:
	${MAKE} -C ./src "$@"
	rm -rf docs

.PHONY: all scan-build lilydumper doc book documentation clean
