# BLACKOUT: SIGNAL LOST — Unreal Engine 5

This branch contains the isolated UE5 authoring project and the automated opening cinematic. The PS4/OpenOrbis branch remains separate.

## Project layout

- `BLACKOUT-UE5.uproject`: UE 5.4 project descriptor.
- `Config/DefaultEngine.ini`: sets `/Game/Maps/L_Intro` as editor and game startup map.
- `Content/Python/create_blackout_intro.py`: creates the intro level, Level Sequence, original generated ambience and auto-play actor inside Unreal Editor.
- `UE5/INTRO_SEQUENCE_PLAN.md`: timeline and creative direction.

## Build the intro assets inside Unreal Editor

1. Open `BLACKOUT-UE5.uproject` with Unreal Engine 5.4.
2. Enable **Python Editor Script Plugin** and **Editor Scripting Utilities**, then restart if requested.
3. Run `Content/Python/create_blackout_intro.py` using **Tools > Execute Python Script**.
4. The script creates `/Game/Maps/L_Intro`, `/Game/Cinematics/LS_BLACKOUT_Intro` and imports its generated sound into `/Game/Audio/SW_BLACKOUT_Intro`.
5. Open the level and preview the Level Sequence. The sequence actor is configured for auto-play; if the installed UE 5.4 patch rejects the auto-play property or camera binding, correct that property/binding in the editor and save.

## Cinematic contents

- 24 seconds at 24 fps (576 frames), with a smooth-key interpolation attempt and a slow forward CineCamera dolly.
- Industrial corridor blockout, cold point lights and atmospheric height fog.
- Opening and ending camera fades where the engine exposes the fade channel.
- Animated `UNREAL ENGINE` text credit, `BLACKOUT`, `SIGNAL LOST`, and `ÉDITÉ PAR RIDAGAMES`.
- Brief offset red/cyan signal-interference text bursts.
- A procedurally generated, original 24-second ambience WAV: low industrial drone, radio-static bursts, a metallic impact and fade-out. It is imported and attached to the sequence when the UE audio scripting API accepts the track.
- The intro map is configured as the project's default startup map and contains a Level Sequence Actor set to auto-play.

## Important status

This is a **complete first-pass scripted intro sequence**, not a finished AAA cinematic. The corridor still uses basic engine geometry; it needs final authored environment meshes, materials, high-quality particle work, a true rendered glitch/compositing pass, final sound mastering and a real transition to the main menu. The script has not been executed in Unreal Editor in this environment, so its UE 5.4 API calls, generated assets, camera cut, audio track and playback must still be validated in the editor. No rendered movie or PS4 build has been produced here.

The UE5 authoring project is separate from the OpenOrbis PS4 homebrew code. UE5 does not become PS4-compatible merely by adding OpenOrbis libraries; a substantial engine/platform adaptation and a compatible build toolchain are still required. Console packaging also requires authorized platform access where applicable.
