# BLACKOUT: SIGNAL LOST — Unreal Engine 5

This branch contains the isolated UE5 authoring project and the opening-cinematic editor automation.

## Project layout

- `BLACKOUT-UE5.uproject`: project descriptor (Unreal Engine 5.4).
- `Config/`: project and renderer settings.
- `Content/Python/create_blackout_intro.py`: builds the intro level and a 24-second Level Sequence inside Unreal Editor.
- `UE5/INTRO_SEQUENCE_PLAN.md`: creative timeline and sound direction.

## Create the intro inside Unreal Editor

1. Open `BLACKOUT-UE5.uproject` with Unreal Engine 5.4.
2. Enable the **Python Editor Script Plugin** and **Editor Scripting Utilities** if prompted, then restart the editor.
3. In Unreal Editor, run `Content/Python/create_blackout_intro.py` from **Tools > Execute Python Script** (menu wording can vary by UE version).
4. Open `/Game/Cinematics/LS_BLACKOUT_Intro` in Sequencer and preview it. The generated level is `/Game/Maps/L_Intro`.
5. If the camera-cut binding is not accepted by the installed UE 5.4 patch, add a Camera Cuts track and assign `INTRO_CinematicCamera` manually.

## What this first scene creates

- A long industrial corridor blockout with floor, ceiling, wall ribs and an end wall.
- Cold practical point lights and atmospheric height fog.
- A Cine Camera with a 24 fps, 24-second forward dolly.
- Opening fade from black (0–2 s) and final fade to black (21–24 s), where supported by the UE version.
- Sequenced scale reveals for `BLACKOUT`, `SIGNAL LOST`, and `ÉDITÉ PAR RIDAGAMES` (staggered between seconds 11 and 19).

This is a **first editor-generated cinematic blockout**, not a finished AAA cinematic. The generated geometry uses Unreal's basic cube mesh; final environment art, authored materials, a dedicated signal-glitch effect, original audio and the actual main-menu handoff remain to be produced and previewed in the editor. The fade track prepares a fade-to-black ending but does not itself load the menu. No rendered result has been tested in this environment.

## Platform note

The UE5 authoring project is separate from the PS4 homebrew code. A PS4/GoldHEN build is not implied: console packaging requires authorized Sony platform access and a compatible engine/toolchain port.
