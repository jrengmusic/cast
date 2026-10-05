# cast v0.1.0

Codegen Annotated Source of Truth

## What's New

- Release lane: a signed macOS pkg and Windows x64 and arm64 installers, uploaded to the GitHub release.
- cast --sync syncs submodules deterministically from the submodule and region tables; a file can carry many regions.
- Signing data lives in cast/signing.md, and entitlements.plist is generated from it.
- description is the reserved documentation column; an unknown bracketed name is a fatal diagnostic.

## Platforms

| Platform | Architecture       | Format                            |
| -------- | ------------------ | --------------------------------- |
| macOS    | universal          | .pkg (signed, notarized)          |
| Windows  | x64                | .exe installer                    |
| Windows  | arm64              | .exe installer                    |

## Installation

macOS: open the .pkg. It installs `cast` on your PATH.

Windows: run the installer. It installs `cast.exe` and adds its folder to your user PATH.
