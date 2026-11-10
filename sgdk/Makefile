
# Installer SGDK, puis: make GDK=/chemin/vers/sgdk
ifndef GDK
$(error Definir GDK vers une installation SGDK, ex. make GDK=/opt/sgdk)
endif

EXTRA_FLAGS += -std=c99

include $(GDK)/makefile.gen

# SGDK fournit deja le code de demarrage et l'en-tete de la ROM.
SRC_S := $(filter-out src/boot/sega.s,$(SRC_S))
SRC_C := $(filter-out src/boot/rom_head.c src/rom_header.c,$(SRC_C))
# SGDK 2.11 materialise OBJS pendant l'include : filtrer aussi les objets.
# sega.s incorpore out/rom_head.bin et fournit deja le symbole rom_header.
# SGDK 2.11 names the build directory OUT, SGDK 2.12 OUT_DIR: filter both.
VK_OUT := $(or $(OUT_DIR),$(OUT))
OBJS := $(filter-out $(VK_OUT)/src/boot/sega.o $(VK_OUT)/src/boot/rom_head.o $(VK_OUT)/src/rom_header.o,$(OBJS))

# Rebuild the link list when this Makefile changes its source filters.
$(VK_OUT)/cmd_: Makefile

# Standard header after SGDK's padding: ROM end and the Sega word-sum checksum
# (sizebnd writes an XOR of its own, which emulators report as wrong).
release: fixHeader
.PHONY: fixHeader
fixHeader: padROM
	python3 ../tools/md_rom_header.py $(VK_OUT)/rom.bin out/rom.bin
	# Tools read the ELF at out/rom.out (SGDK 2.11 layout); SGDK 2.12 writes out/release/rom.out.
	@[ "$(VK_OUT)" = out ] || cp $(VK_OUT)/rom.out out/rom.out
