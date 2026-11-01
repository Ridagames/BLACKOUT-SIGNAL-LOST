#!/usr/bin/env bash
set -euo pipefail

PKG="${1:-blackout.pkg}"
SOURCE_PKG="${2:-blackout.gnpdrm.pkg}"

echo "=== Vérification du package HEN ==="
test -s "$PKG"
test -s "$SOURCE_PKG"
test -s build/pkg/USRDIR/EBOOT.BIN
test -s build/pkg/PARAM.SFO

pkg_size=$(stat -c %s "$PKG")
source_size=$(stat -c %s "$SOURCE_PKG")
test "$pkg_size" -gt 100000
test "$pkg_size" -lt 200000000
test "$source_size" -gt 100000
test "$source_size" -lt 200000000

if ! cmp -s "$PKG" "$SOURCE_PKG"; then
  echo "ERREUR: $PKG n'est pas la copie finalisée de $SOURCE_PKG"
  exit 1
fi

echo "PKG finalisé : $PKG ($pkg_size octets)"
echo "EBOOT.BIN :"
file build/pkg/USRDIR/EBOOT.BIN
echo "PARAM.SFO :"
file build/pkg/PARAM.SFO
echo "SHA-256 :"
sha256sum "$PKG" "$SOURCE_PKG" build/pkg/USRDIR/EBOOT.BIN build/pkg/PARAM.SFO
echo "Vérification terminée."
