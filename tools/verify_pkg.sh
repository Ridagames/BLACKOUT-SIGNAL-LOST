#!/usr/bin/env bash
set -euo pipefail

PKG="${1:-blackout.pkg}"
SOURCE_PKG="${2:-blackout.gnpdrm.pkg}"
EBOOT="build/pkg/USRDIR/EBOOT.BIN"
SFO="build/pkg/PARAM.SFO"
EXPECTED_CONTENT_ID="UP0001-BLSG00001_00-0000000000000000"

echo "=== Vérification des fichiers ==="
for f in "$PKG" "$SOURCE_PKG" "$EBOOT" "$SFO"; do
  test -s "$f" || { echo "ERREUR: fichier absent ou vide: $f"; exit 1; }
done

echo "=== Vérification du package finalisé ==="
if ! cmp -s "$PKG" "$SOURCE_PKG"; then
  echo "ERREUR: le PKG publié diffère du PKG GNPDRM finalisé"
  exit 1
fi

echo "=== Vérification de la magie SELF ==="
magic=$(od -An -tx1 -N4 "$EBOOT" | tr -d ' \n')
test "$magic" = "53434500" || {
  echo "ERREUR: EBOOT.BIN n'a pas la signature SELF 53 43 45 00 (magie trouvée: $magic)"
  exit 1
}

echo "=== Vérification de l'identifiant NPDRM dans EBOOT ==="
grep -aFq "$EXPECTED_CONTENT_ID" "$EBOOT" || {
  echo "ERREUR: CONTENT_ID attendu absent de EBOOT.BIN"
  exit 1
}

echo "=== Analyse PARAM.SFO ==="
python3 - "$SFO" "$EXPECTED_CONTENT_ID" <<'PY'
import struct, sys
path, expected_content_id = sys.argv[1], sys.argv[2]
data = open(path, "rb").read()
if len(data) < 20 or data[:4] != b"\x00PSF":
    raise SystemExit("ERREUR: signature PARAM.SFO invalide")
_, key_off, data_off, count = struct.unpack_from("<4I", data, 0)
entries = {}
for i in range(count):
    pos = 20 + i * 16
    if pos + 16 > len(data):
        raise SystemExit("ERREUR: table SFO tronquée")
    key_rel, fmt, length, max_len, value_rel = struct.unpack_from("<HHIII", data, pos)
    kp, vp = key_off + key_rel, data_off + value_rel
    if kp >= len(data) or vp + length > len(data):
        raise SystemExit("ERREUR: offset SFO hors limites")
    end = data.find(b"\0", kp)
    if end < 0:
        raise SystemExit("ERREUR: clé SFO non terminée")
    key = data[kp:end].decode("ascii", "replace")
    raw = data[vp:vp+length].split(b"\0", 1)[0]
    entries[key] = raw.decode("utf-8", "replace")
for key in ("CATEGORY", "TITLE_ID", "CONTENT_ID"):
    if key not in entries:
        raise SystemExit("ERREUR: champ SFO manquant: " + key)
if entries["CATEGORY"] != "HG":
    raise SystemExit("ERREUR: CATEGORY doit être HG, reçu " + repr(entries["CATEGORY"]))
if entries["TITLE_ID"] != "BLSG00001":
    raise SystemExit("ERREUR: TITLE_ID inattendu: " + repr(entries["TITLE_ID"]))
if entries["CONTENT_ID"] != expected_content_id:
    raise SystemExit("ERREUR: CONTENT_ID SFO inattendu: " + repr(entries["CONTENT_ID"]))
print("CATEGORY =", entries["CATEGORY"])
print("TITLE_ID =", entries["TITLE_ID"])
print("CONTENT_ID =", entries["CONTENT_ID"])
PY

echo "=== Outils de dépaquetage SELF ==="
if command -v unself >/dev/null 2>&1; then
  echo "unself disponible: $(command -v unself)"
  tmp=$(mktemp -d)
  trap 'rm -rf "$tmp"' EXIT
  unself "$EBOOT" "$tmp/EBOOT.ELF"
  test -s "$tmp/EBOOT.ELF"
  sha256sum "$tmp/EBOOT.ELF"
else
  echo "AVERTISSEMENT: unself absent; contrôle de déchiffrement SELF non exécuté."
fi

echo "=== Empreintes ==="
sha256sum "$PKG" "$SOURCE_PKG" "$EBOOT" "$SFO"
echo "Vérification terminée."
