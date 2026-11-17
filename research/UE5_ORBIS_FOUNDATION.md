# UE 5.8.4 → OpenOrbis: source foundation progress

Work is isolated in the private Unreal Engine fork:

- Repository: `Ridagames/UnrealEngine`
- Branch: `ridagames/ps4-openorbis-port`
- No changes to Unreal `release`, BLACKOUT `main`, or BLACKOUT `ps4/port`.

## Added source foundations

### UnrealBuildTool
- `Engine/Source/Programs/UnrealBuildTool/Platform/Orbis/OrbisSDKLayout.cs`
  - Reads `OO_PS4_TOOLCHAIN` / `OPENORBIS_SDK`.
  - Locates host Clang, LLD and llvm-ar from overrides or PATH.
  - Resolves candidate SDK include and library directories.
- `OrbisToolChain.cs`
  - Subclasses UE 5.8.4 `ClangToolChain`.
  - Adds `x86_64-pc-freebsd12-elf`, LLD selection and SDK include/library paths.
  - Not yet connected to a registered target platform.

### Core platform layer
- `Core/Public/Orbis/OrbisPlatform.h`: initial Orbis / FreeBSD-ABI platform definitions. It intentionally avoids including UE's `UnixPlatform.h`, which imports Linux-specific kernel headers.
- `OrbisPlatformProperties.h`: initial cooked-console properties.
- `OrbisPlatformMisc.h`: declaration boundary; platform implementations are not present yet.

### Rendering boundary
- `Runtime/OrbisRHI/OrbisRHI.Build.cs`
- `Runtime/OrbisRHI/Private/OrbisRHI.cpp`

These define a module boundary only. They do not implement `FDynamicRHI`, Piglet/EGL, GLES rendering, swapchain, or PS4 shader handling.

## Blocking integration work still required

1. Add `Orbis` to the actual `UnrealTargetPlatform` enumeration and DataDrivenPlatformInfo.
2. Implement the UBT platform factory, SDK validation/version logic, deploy and cook target.
3. Complete and validate Clang/LLD flags against the exact installed OpenOrbis SDK, including sysroot, C++ runtime, linker scripts/entry point and ELF output.
4. Wire Core platform selection and implement platform startup, memory, threading, filesystem, input and audio.
5. Implement the actual Orbis RHI using the Piglet/EGL/GLES path and the PS4 shader pipeline.
6. Add cooker, packaging, signing and deployment.

**Status:** source foundations only. No build, test, workflow or PS4 execution was run. This is not yet a functioning UE PS4 port.
