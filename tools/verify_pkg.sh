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
if len(data) < 20:
    raise SystemExit("ERREUR: PARAM.SFO trop court")
magic, version, key_off, data_off, count = struct.unpack_from("<5I", data, 0)
if magic != 0x46535000:
    raise SystemExit("ERREUR: signature PARAM.SFO invalide")
if not (0 < count < 256):
    raise SystemExit(f"ERREUR: nombre d'entrees SFO aberrant: {count}")
entries = {}
for i in range(count):
    pos = 20 + i * 16
    if pos + 16 > len(data):
        raise SystemExit("ERREUR: table SFO tronquee")
    key_rel, fmt, length, max_len, value_rel = struct.unpack_from("<HHIII", data, pos)
    kp, vp = key_off + key_rel, data_off + value_rel
    if kp >= len(data) or vp + length > len(data):
        raise SystemExit("ERREUR: offset SFO hors limites")
    end_key = data.find(b"\\0", kp)
    if end_key < 0:
        raise SystemExit("ERREUR: cle SFO non terminee")
    key = data[kp:end_key].decode("ascii", "replace")
    raw = data[vp:vp+length]
    if fmt == 4:
        if len(raw) != 4:
            raise SystemExit("ERREUR: entier SFO de taille invalide: " + key)
        entries[key] = struct.unpack("<i", raw)[0]
    else:
        entries[key] = raw.split(b"\\0", 1)[0].decode("utf-8", "replace")
required = ("ATTRIBUTE","BOOTABLE","CATEGORY","CONTENT_ID","PARENTAL_LEVEL",
            "PS3_SYSTEM_VER","RESOLUTION","SOUND_FORMAT","TITLE","TITLE_ID",
            "VERSION","LICENSE")
missing = [k for k in required if k not in entries]
if missing:
    raise SystemExit("ERREUR: champs SFO manquants: " + ", ".join(missing))
if entries["CATEGORY"] != "HG":
    raise SystemExit("ERREUR: CATEGORY doit etre HG, recu " + repr(entries["CATEGORY"]))
if entries["BOOTABLE"] != 1:
    raise SystemExit("ERREUR: BOOTABLE doit valoir 1, recu " + repr(entries["BOOTABLE"]))
if entries["TITLE_ID"] != "BLSG00001":
    raise SystemExit("ERREUR: TITLE_ID inattendu: " + repr(entries["TITLE_ID"]))
if entries["CONTENT_ID"] != expected_content_id:
    raise SystemExit("ERREUR: CONTENT_ID SFO inattendu: " + repr(entries["CONTENT_ID"]))
if entries["PS3_SYSTEM_VER"] > "04.9100":
    raise SystemExit("ERREUR: PS3_SYSTEM_VER " + entries["PS3_SYSTEM_VER"] + " > 04.9100")
print("CATEGORY =", entries["CATEGORY"])
print("BOOTABLE =", entries["BOOTABLE"])
print("TITLE_ID =", entries["TITLE_ID"])
print("CONTENT_ID =", entries["CONTENT_ID"])
print("PS3_SYSTEM_VER =", entries["PS3_SYSTEM_VER"])
PY

echo "=== Déchiffrement et comparaison SELF/ELF ==="
command -v unself >/dev/null 2>&1 || {
  echo "ERREUR: unself absent; vérification SELF obligatoire impossible"
  exit 1
}
command -v ppu-readelf >/dev/null 2>&1 || {
  echo "ERREUR: ppu-readelf absent; vérification ELF impossible"
  exit 1
}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
unself "$EBOOT" "$tmp/EBOOT.ELF"
test -s "$tmp/EBOOT.ELF" || {
  echo "ERREUR: unself n'a pas produit EBOOT.ELF"
  exit 1
}
test "$(od -An -tx1 -N4 "$tmp/EBOOT.ELF" | tr -d ' \\n')" = "7f454c46" || {
  echo "ERREUR: le SELF dechiffre n'est pas un ELF"
  exit 1
}

# Le point d'entree et le decoupage en segments doivent etre identiques.
if ! diff -u \\
  <(ppu-readelf -h build/blackout.elf | awk '/Entry point/{print $4}'
    ppu-readelf -l build/blackout.elf | awk '$1=="LOAD"{print $2,$3,$5}') \\
  <(ppu-readelf -h "$tmp/EBOOT.ELF" | awk '/Entry point/{print $4}'
    ppu-readelf -l "$tmp/EBOOT.ELF" | awk '$1=="LOAD"{print $2,$3,$5}'); then
  echo "ERREUR: le SELF dechiffre ne correspond pas a l'ELF compile"
  exit 1
fi

# Comparaison octet a octet informative seulement.
if cmp -s "$tmp/EBOOT.ELF" build/blackout.elf; then
  echo "SELF dechiffre identique octet pour octet a build/blackout.elf"
else
  echo "NOTE: difference d'octets (table des sections probablement retiree)"
  ls -l "$tmp/EBOOT.ELF" build/blackout.elf
fi
sha256sum "$tmp/EBOOT.ELF" build/blackout.elf

echo "=== Empreintes ==="
sha256sum "$PKG" "$SOURCE_PKG" "$EBOOT" "$SFO"
echo "Vérification terminée."
