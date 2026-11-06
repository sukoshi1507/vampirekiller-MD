.PHONY: build verify-build verify
PYTHON ?= python3

# GDK and PREFIX are inherited by tools/build.py, or can be set on this command.
export GDK PREFIX
PREFIX ?= m68k-elf-

build:
	$(PYTHON) tools/build.py

verify-build:
	$(PYTHON) tools/build.py --verify-reproducible

verify:
	$(PYTHON) tools/verify_baseline.py --rom "$(SOURCE_ROM)" --core "$(LIBRETRO_CORE)"
