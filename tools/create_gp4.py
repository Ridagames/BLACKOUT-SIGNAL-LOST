#!/usr/bin/env python3
"""Create a GP4 project for the BLACKOUT PS4 package using only Python's stdlib."""
import argparse
import datetime
import shutil
import xml.etree.ElementTree as ET
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument("--content-id", required=True)
p.add_argument("--title-id", required=True)
p.add_argument("--stage", required=True)
p.add_argument("--eboot", required=True)
p.add_argument("--sfo", required=True)
p.add_argument("--asset", required=True)
p.add_argument("--output", required=True)
a = p.parse_args()
stage = Path(a.stage)
if stage.exists():
    shutil.rmtree(stage)
(stage / "sce_sys").mkdir(parents=True)
(stage / "USRDIR").mkdir(parents=True)
shutil.copy2(a.eboot, stage / "eboot.bin")
shutil.copy2(a.sfo, stage / "sce_sys/param.sfo")
shutil.copy2(a.asset, stage / "USRDIR/BLACKOUT_MENU.png")
files = sorted(str(f.relative_to(stage)).replace("\\\\", "/") for f in stage.rglob("*") if f.is_file())

root = ET.Element("psproject", {"fmt": "gp4", "version": "1000"})
volume = ET.SubElement(root, "volume")
ET.SubElement(volume, "volume_type").text = "pkg_ps4_app"
ET.SubElement(volume, "volume_id").text = "PS4VOLUME"
ET.SubElement(volume, "volume_ts").text = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
ET.SubElement(volume, "package", {
    "content_id": a.content_id, "passcode": "0" * 32,
    "storage_type": "digital50", "app_type": "full"
})
info = ET.SubElement(volume, "chunk_info", {"chunk_count": "1", "scenario_count": "1"})
ET.SubElement(ET.SubElement(info, "chunks"), "chunk", {"id": "0", "layer_no": "0", "label": "Chunk #0"})
scenarios = ET.SubElement(info, "scenarios", {"default_id": "0"})
ET.SubElement(scenarios, "scenario", {
    "id": "0", "type": "sp", "initial_chunk_count": "1", "label": "Scenario #0"
}).text = "0"
listing = ET.SubElement(root, "files", {"img_no": "0"})
for f in files:
    ET.SubElement(listing, "file", {"targ_path": f, "orig_path": f})

def add_dirs(parent, folder):
    for d in sorted(x for x in folder.iterdir() if x.is_dir()):
        add_dirs(ET.SubElement(parent, "dir", {"targ_name": d.name}), d)

add_dirs(ET.SubElement(root, "rootdir"), stage)
ET.indent(root, "\\t")
Path(a.output).write_text('<?xml version="1.0"?>\\n' + ET.tostring(root, encoding="unicode") + "\\n", encoding="utf-8")
print("GP4 created:", a.output, "files:", len(files))
