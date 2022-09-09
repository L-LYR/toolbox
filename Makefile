.PHONY: clean all coverage test build

all: build
	meson compile -C build

build:
	CC=clang CXX=clang++ meson build

test:
	meson test toolbox: -C build -t -1

coverage:
	CC=clang CXX=clang++ meson build-cov -Db_coverage=true && \
	meson compile -C build-cov && \
	meson test toolbox: -C build-cov -t -1 && \
	ninja coverage -C build-cov

sanitize:
	CC=clang CXX=clang++ meson build-sanitize -Db_sanitize=address -Db_lundef=false && \
	meson compile -C build-sanitize && \
	meson test toolbox: -C build-sanitize -t -1

clean:
	rm -rf build build-coverage build-sanitize
