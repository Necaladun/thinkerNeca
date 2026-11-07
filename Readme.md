
# thinkerNeca — Alpha Centauri / Alien Crossfire Thinker fork

**This is Necaladun's experimental fork of [Induktio's Thinker Mod](https://github.com/induktio/thinker)**, not an upstream Thinker release. Its current focus is independent minor terrain, major-landmark generation, graphics integration and adjustable terrain economics for **fresh SMAC/X games**. Draft [PR #4](https://github.com/Necaladun/thinkerNeca/pull/4) carries the latest work; the fork is **not yet a validated Release build**.

The new independent minor features are **Jungle (+1 nutrient), Uranium (+1 energy) and Titanium (+1 mineral)**, rendered without consuming original major-landmark slots. Uranium and Titanium use the same native resource sprite provisionally, with an alternating shared geology generator that keeps the two deposit types apart. Nessus Canyon's generator has been retired; major slot 10 is reserved. The distributed `docs/thinker.ini` is a **specific gameplay test profile**, currently using 3/3/3 resource caps, additional starting units and selected event exclusions.

**Project navigation:** [What is happening now?](project/STATUS.md) · [Work queue / WIP / TODO / TBD](project/WORK-QUEUE.md) · [Fork changelog](Fork-Changelog.md) · [Testing](project/TESTING.md) · [Project guide](PROJECT.md) · [Decisions](project/DECISIONS.md) · [Ideas](project/IDEAS.md).

**Detailed mechanics:** [Independent terrain](project/technical/minor-terrain.md) · [Native graphics](project/technical/rendering.md) · [Major landmarks](project/technical/map-generation.md) · [INI configuration](project/technical/configuration.md).

## Fork build and installation

For testing use the **Windows 32-bit Develop** artifacts from the [fork's GitHub Actions workflow](https://github.com/Necaladun/thinkerNeca/actions/workflows/build-windows.yml) associated with the intended commit/PR. The CI artifact includes `thinker.dll`, `thinker.exe` and the generated `minor_terrain.pcx`. Put the DLL **and matching PCX** in the SMAC/X game directory with the launcher, and deploy the intended `docs/thinker.ini` plus other game data you elect to use. If you have `thinker_user.ini`, it loads *after* `thinker.ini` and can override your tests. Delete any obsolete `canyon=` line from local overrides.

The fork assumes a supported Alien Crossfire v2.0 `terranx.exe`. Native x86 hooks may be executable-specific. CI compilation/static checks do not replace runtime testing. Start the game from `thinker.exe` and verify the mod version via the Thinker options/game version menu. Older save/scenario compatibility is not an objective, but games must save and reload correctly on the same tested build.

**The material below is inherited Thinker information** and may describe upstream defaults or releases rather than this fork's current configuration. For fork differences use the links above.

## Upstream Thinker overview

Thinker is a remake of game engine features for Alpha Centauri: Alien Crossfire.
By patching the game to use an additional DLL, many new features can be developed in C++.

* Visual changes to map rendering and random map generation
* Additional game mechanics and more detailed config options for the game rules
* Numerous previously fixed settings in the game engine can be adjusted from the config
* More options for resolution settings, movie playback and other user interface features
* Entirely rewritten production and movement AI with minor changes to diplomacy
* Improved automation routines are also available for player factions
* Includes [Scient's patches](Details.md)

This mod is tested to work with the [GOG version](https://www.gog.com/game/sid_meiers_alpha_centauri) of Alpha Centauri.
Note that official Alien Crossfire patch version 2.0 must be installed for the launcher to work with terranx.exe.
Older game version 1.0 is NOT supported by Thinker. Installing Scient's patch v2 also works since this terranx.exe can be used by the launcher.
Thinker also includes changes to remove issues on Windows 11 that prevent playing the game expansion due to crashes after ending the turn.

[See more information](Details.md) about the features and recommended settings.
It's strongly recommended to read Details.md since many features are added not provided by the original game.
[Discuss here](https://github.com/induktio/thinker/discussions) about anything related to Thinker development.
Remember also to star and watch the repository to receive notifications about new updates.


Upstream Thinker downloads
-------------------------
These are **Induktio's upstream Thinker** binaries, not builds of thinkerNeca. The inherited [Changelog](Changelog.md) is upstream history; use the [fork changelog](Fork-Changelog.md) for fork changes.

* [Release versions](https://www.dropbox.com/sh/qsps5bhz8v020o9/AAAp6ioWxdo7vnG6Ity5W3o1a?dl=0&lst=)
* [Develop builds](https://www.dropbox.com/sh/qsps5bhz8v020o9/AADv-0D0-bPq22pgoAIcDRC3a/develop?dl=0&lst=)


Installation
------------
For **this fork**, follow [the fork build instructions above](#fork-build-and-installation). The steps below apply broadly to Thinker's launcher and UI:

1. Use the intended compatible Alien Crossfire v2.0 game installation and matching fork build.
2. Deploy the correct `thinker.dll`, `thinker.exe`, `minor_terrain.pcx` and chosen INI/data files; don't mix atlas and DLL versions.
3. Review `thinker.ini` and any local `thinker_user.ini` overrides.
4. Start the game from `thinker.exe`.
5. Press ALT+T for Thinker's options menu (ALT+H when using PRACX). The version/build date is also visible via CTRL+F4.
6. If Thinker version information is absent, the mod was not loaded correctly.


Troubleshooting
---------------
The launcher requires original Alien Crossfire v2.0 terranx.exe in the same folder but this file is not modified on disk.
In case of startup problems, make sure the official v2.0 patch is applied on the game, and the mod is actually started
from same folder with the game. Sometimes startup issues can be fixed by starting the launcher with administrator privileges.

When starting the game with screen scaling set to something else than 100% some portion of the window may not be visible due to being
clipped out of the screen. The game does not have proper support for screen scaling so as a workaround the scaling should be set to 100%.
Sometimes opening/secret project video playback errors might be fixed by installing [DDrawCompat](https://github.com/narzoul/DDrawCompat)
but otherwise it is not necessary.

The current GOG Alpha Centauri Planetary Pack (13 November 2024) may not work on Windows XP by default. When starting the game
there can be an error message "The application failed to start because dwmapi.dll was not found." To fix this simply remove ddraw.dll
from the game folder. This file was added on the latest GOG version and it is not necessary to run the game on Windows XP.

Alt-tabbing may sometimes not work by default in Alpha Centauri's GOG version. To re-enable alt-tab feature, follow these steps.
First open a command prompt with administrator privileges in Alpha Centauri's installation folder. After entering the commands
below alt-tabbing should now work when the game is restarted.

    sdbinst -u game.sdb
    sdbinst -u game_add.sdb

After installing the game on Windows there might be a notification that Windows Features can't complete the requested changes.
This might be caused by a failure to install DirectPlay. First open a command prompt with administrator privileges.
Then after entering the command below DirectPlay should be automatically installed.

    dism /online /Enable-Feature /FeatureName:DirectPlay /All


Other mods
----------
* [SMAC-in-SMACX mod](Details.md#smac-in-smacx-mod) can be installed to play a game similar to original SMAC while Thinker is enabled.
* [Scient's patch](https://github.com/DrazharLn/scient-unofficial-smacx-patch) combines many fixes for the original game.
* [OpenSMACX](https://github.com/b-casey/OpenSMACX) has been a long-term project to recreate SMACX game engine.
* Also see [compatibility with other mods such as PRACX](Details.md#compatibility-with-other-mods).


Compiling
---------
For information on how to compile Thinker, see [Technical.md](Technical.md).


License
-------
This software is licensed under the MIT License. Check [License.md](License.md) for detailed conditions.

The original game assets are not covered by this license and remain property of Firaxis Games Inc and Electronic Arts Inc.

Sid Meier's Alpha Centauri and Sid Meier's Alien Crossfire is Copyright © 1997, 1998 by Firaxis Games Inc and Electronic Arts Inc.
