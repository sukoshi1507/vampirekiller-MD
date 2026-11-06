
# Installer SGDK, puis: make GDK=/chemin/vers/sgdk
ifndef GDK
$(error Definir GDK vers une installation SGDK, ex. make GDK=/opt/sgdk)
endif

EXTRA_FLAGS += -std=c99

include $(GDK)/makefile.gen

# SGDK fournit deja le code de demarrage et l'en-tete de la ROM.
SRC_S := $(filter-out src/boot/sega.s,$(SRC_S))
SRC_C := $(filter-out src/boot/rom_head.c,$(SRC_C))

# Standard header after SGDK's padding: ROM end and the Sega word-sum checksum
# (sizebnd writes an XOR of its own, which emulators report as wrong).
release: fixHeader
.PHONY: fixHeader
fixHeader: padROM
	python3 ../tools/md_rom_header.py $(OUT_DIR)/rom.bin out/rom.bin
