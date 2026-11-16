# BLACKOUT: SIGNAL LOST — Opening cinematic

Target: 24 seconds, 16:9, 24 fps. Mood: oppressive industrial survival horror.

| Time | Picture | Sound |
|---|---|---|
| 00:00–00:02 | Black frame fades into a cold industrial corridor. | Original low industrial drone fades in. |
| 00:02–00:07 | Short studio/engine credit, camera begins its slow forward dolly. | Drone and distant machine harmonics. |
| 00:07–00:11 | Red/cyan signal echoes flicker in short bursts. | Original radio-static bursts. |
| 00:11–00:16 | BLACKOUT title resolves, followed by SIGNAL LOST. | Low metallic impact and sustained drone. |
| 00:16–00:21 | ÉDITÉ PAR RIDAGAMES appears and holds. | Drone continues, slowly receding. |
| 00:21–00:24 | Fade to black, ready for the next game state. | Original ambience fades out. |

## Implemented by the editor automation

The UE 5.4 Python script builds the corridor blockout, cold practical lights, atmospheric height fog, CineCamera, Level Sequence, animated studio/title/credit text, short red/cyan signal-interference text bursts and opening/ending fade keys where supported. It generates an original 24-second WAV ambience (industrial drone, radio-static bursts, metallic impact) and attempts to import it and attach it to the sequence. The generated intro map is configured as the project's startup map and a Level Sequence Actor is set to auto-play.

## Still required before calling it final

- Execute the script in Unreal Editor 5.4 and fix any version-specific Python API issues.
- Preview the camera cut, keyframe interpolation, fade track and audio playback; render a review movie.
- Replace the basic cube corridor with final authored environment art and materials.
- Add polished Niagara dust, volumetric lighting and a true post-process signal-glitch effect.
- Master and mix final original music and sound effects.
- Build the actual main-menu map and implement a reliable transition from the final fade into that menu.

The current source is a complete first-pass cinematic automation, not a verified rendered result. The UE5 branch remains isolated from the OpenOrbis PS4 branch.
