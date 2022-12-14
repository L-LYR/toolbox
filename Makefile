.PHONY: clean all coverage test build

all: build
	meson compile -C build

build:
	CC=clang CXX=clang++ meson build

test:
	meson build --reconfigure -Dbuildtype=debug && \
	meson test toolbox: -C build -v -t -1 --no-rebuild

bench:
	meson build --reconfigure -Dbuildtype=release && \
	meson test toolbox: -C build -v -t -1 --benchmark --no-rebuild

coverage:
	CC=clang CXX=clang++ meson build-cov -Db_coverage=true && \
	meson compile -C build-cov && \
	meson test toolbox: -C build-cov -t -1 --no-rebuild && \
	ninja coverage -C build-cov

sanitize:
	CC=clang CXX=clang++ meson build-sanitize -Db_sanitize=address -Db_lundef=false && \
	meson compile -C build-sanitize && \
	meson test toolbox: -C build-sanitize -t -1 --no-rebuild

clean:
	rm -rf build build-cov build-sanitize
