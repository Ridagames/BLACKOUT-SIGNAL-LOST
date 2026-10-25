# Minimal PSL1GHT application Makefile.
# Les règles globales sont fournies par l'environnement PS3DEV/PSL1GHT.
# Selon la version du SDK, adaptez les noms des variables/cibles au template
# officiel installé dans PSL1GHT/templates.

APPNAME       := blackout
TITLE         := BLACKOUT: SIGNAL LOST
CONTENTID     := BLUS00000
TARGET        := $(APPNAME).elf

# Échoue avec un message clair si le toolchain n'est pas chargé.
.PHONY: all check-env clean
all: check-env
	@echo "PS3DEV detected. Connect this project to the installed PSL1GHT template rules."
	@echo "See README.md; this starter intentionally does not fake a PKG."

check-env:
	@test -n "$$PS3DEV" || (echo "ERROR: PS3DEV is not set"; exit 1)
	@test -n "$$PSL1GHT" || (echo "ERROR: PSL1GHT is not set"; exit 1)
	@command -v ppu-gcc >/dev/null 2>&1 || (echo "ERROR: ppu-gcc not found in PATH"; exit 1)

clean:
	rm -f *.elf *.self *.pkg *.o
