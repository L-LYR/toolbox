.PHONY: clean all coverage test build

all: build
	meson compile -C build

build:
	CC=clang CXX=clang++ meson build

test:
	meson test toolbox: -C build -t -1

coverage:
	CC=clang CXX=clang++ meson build-cov -Db_coverage=true && \
	meson test toolbox: -C build-cov -t -1 && \
	ninja coverage -C build-cov

clean:
	rm -rf build*
