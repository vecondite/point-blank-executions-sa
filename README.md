# Point-Blank Executions for GTA: San Andreas
![Static Badge](https://img.shields.io/badge/GTA%3ASA-v1.0%20HOODLUM-blue?style=for-the-badge&logo=rockstargames&logoColor=%23FCAF17&logoSize=auto)
A lightweight, highly configurable mod inspired by *Red Dead Redemption* that lets you perform cinematic point-blank executions on pedestrians.

<img width="800" height="450" alt="ezgif-539ebd5f917d2ae5" src="https://github.com/user-attachments/assets/eeecb915-2840-4e06-8290-a19e72ad541f" />
<img width="800" height="450" alt="ezgif-5c2abb3cf3482145" src="https://github.com/user-attachments/assets/cefa5b2d-c958-4fc5-9405-faf02db77e6e" />

## Features
- **Weapon-Specific Configs:** Configure different radii, dot-product angles, delays, and custom animations for every weapon type via `weapons.txt`.
- **Cinematic Camera:** Optional dynamic camera angles during execution.
- **Hot-Reloading:** Modify `config.ini` or `weapons.txt` on the fly and reload in-game using your custom cheat code.
- **Customizable HUD:** Fully customizable target indicator shape and colors.

---

## Installation
1. Drop the `modloader` folder to your `GTA:SA` root folder.

---

## How to Use
1. Equip any supported weapon and walk up close to a pedestrian.
2. Hold **Z** (default keybind) while aiming near them. A custom indicator will appear above their head when locked on.
3. Release the key to trigger the execution sequence.

---

## Configuration

### `config.ini`
Controls global settings, keybinds, reload cheats, and HUD indicator styling (bone attachment, offsets, colors, and quad vertices).

### `weapons.txt` Format
Each line in `weapons.txt` represents a weapon execution profile. Duplicate weapon IDs are supported (the first matching dot-product range takes priority).

**Column Order:**
1. `weaponId` (integer)
2. `Radius` (float)
3. `LowerDotProduct` (float)
4. `UpperDotProduct` (float)
5. `Slowness` (float - time scale multiplier)
6. `ShootDelay` (integer, ms)
7. `GiveBackControlDelay` (integer, ms)
8. `SlownessDelay` (integer, ms)
9. `SlownessDuration` (integer, ms)
10. `CinematicCamera` (Boolean: 1 or 0)
11. `CamX`, `CamY`, `CamZ` (float offsets)
12. `VictimIFP` (string)
13. `VictimAnim` (string)
14. `PlayerIFP` (string)
15. `PlayerAnim` (string)
16. `TargetBone` (integer ID)
17. `DieDelay` (integer, ms)

> ⚠️ **Warning:** Error handling is minimal. Ensure your `weapons.txt` strictly follows the 19-column space/tab-separated format, or the game may crash on load.

---

## Contributions Needed
I am absolutely horrible at animating! Custom animation files and balanced `weapons.txt` presets are heavily appreciated. Feel free to reach out and share your work.

---

## Credits & Acknowledgments
- **vecondite** — Creator / Developer
- **Gemini** — Bug hunting, Plugin-SDK method discovery, and default config generation, also for overhauling this readme.
- **metayeti** — mINI library for configuration parsing.
- **Dryxio** — Animation editor.

Distributed under the **MIT License**.

---

## Contact & Bug Reports
Found a bug or have a suggestion? Reach out via:
- **Discord:** `vecondite`
- **GitHub:** `vecondite`
- **LibertyCity:** `vecondite`
