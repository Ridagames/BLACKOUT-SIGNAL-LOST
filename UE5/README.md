# BLACKOUT: SIGNAL LOST — Unreal Engine 5

This branch isolates the UE5 project scaffold from the existing OpenOrbis PS4 homebrew branch.

## Current state

- A dedicated UE5 project descriptor is provided at `BLACKOUT-UE5.uproject`.
- Python Editor Scripting and Editor Scripting Utilities are enabled for editor automation.
- The intro sequence is being built as an Unreal Editor scene/Level Sequence. This repository commit is source scaffolding; it is **not** a rendered or tested cinematic.

## Important platform note

The UE5 project is for authoring and preview on a supported desktop editor. A PS4 build is a separate platform port and requires authorized Sony platform access and compatible engine/toolchain work. Do not assume this project can currently package for PS4/GoldHEN.

## Intro direction

Target runtime: about 24 seconds. Tone: oppressive survival-horror, near-black image, distant industrial ambience, cold volumetric shafts, restrained glitch pulses, camera push through darkness, title reveal, then `ÉDITÉ PAR RIDAGAMES`. Use original audio only; no Unreal/Epic logo artwork is bundled or imitated. If an official Unreal Engine logo is used, follow Epic's branding guidelines.
