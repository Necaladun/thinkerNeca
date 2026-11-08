#!/usr/bin/env python3
"""AUTO-LAUNCH new-world map/rules contract; real Windows game tests pending.

Do not conflate the launcher checkboxes with a working native new-game path.
The game runs the same native map init, built-in map loading, and rule flags.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
get = lambda path: (root / path).read_text(encoding="utf-8")
ps = get("Run Automation.ps1")
main = get("src/main.cpp")
hdr = get("src/main.h")
game = get("src/game.cpp")
enums = get("src/engine_enums.h")
alpha = get("docs/alphax.txt")

def check(label, condition):
    assert condition, label
    print("PASS:", label)

world = alpha.split("#WORLDSIZE", 1)[1].split("\n#", 1)[0]
check("Very Huge is the actual sixth alpha.txt preset",
      re.search(r"(?m)^6\s*$", world) is not None
      and "Very Huge Planet|(80x160),    80, 160" in world)
check("native Planet files use distinct stock map loading routes",
      'mod_load_map_daemon("maps\\\\xplanet.mp")' in game
      and 'mod_load_map_daemon("maps\\\\xplanetx.mp")' in game
      and "DefaultPrefs->map_type = autosim_new_world_source" in game
      and "autosim_new_world_source + 1 : 1;" in game
      and 'FileExists(builtin)' in game)
check("missing or invalid installed world/map data fails closed in launcher mode",
      "selection < 0 || selection >= entry_count" in game
      and "Selected world size is missing" in game
      and "no #WORLDSIZE list" in game
      and 'if (mod_load_map_daemon("maps\\\\xplanet.mp")) {' in game
      and 'if (mod_load_map_daemon("maps\\\\xplanetx.mp")) {' in game
      and "Could not load maps\\\\xplanet.mp." in game
      and "Could not load maps\\\\xplanetx.mp." in game)
map_menu = game.split("int __cdecl map_menu(int flag)", 1)[1].split(
    "int __cdecl top_menu(int flag)", 1
)[0]
check("automation failed setup exits instead of looping an interactive menu",
      map_menu.count("if (autosim_auto_new_game) return 1;") >= 3
      and "if (config_game(flag)) {" in map_menu
      and "if (size_of_planet(flag)) {" in map_menu)
check("generated custom dimensions bypass the interactive native edit dialog",
      "autosim_new_world_source == 0 && autosim_new_world_size == 6" in game
      and "*MapSizePlanet = 99;" in game
      and "*MapAreaX = 2 * autosim_new_custom_x;" in game
      and "*MapAreaY = autosim_new_custom_y;" in game
      and "map_init();" in game
      and "Path_init(Paths);" in game)
check("world profile supports legacy 0..4 sizes, new 5, custom 6",
      'autosim_profile_int(value, 6, &p->size)' in main
      and 'autosim_profile_int(value, 2, &p->world_source)' in main
      and 'autosim_profile_int(value, 512, &p->custom_x)' in main
      and 'autosim_profile_int(value, 512, &p->custom_y)' in main
      and "p.size == 6" in main
      and "p.custom_x < 16 || p.custom_y < 16" in main)
check("new map routes exist in one WinForms launcher",
      "New-Tab 'World generation'" in ps
      and "Very Huge (80 x 160)" in ps
      and "Custom dimensions" in ps
      and "Map of Planet (maps\\xplanet.mp)" in ps
      and "Huge Map of Planet (maps\\xplanetx.mp)" in ps
      and "world_source=[string][int]$worldSource.SelectedIndex" in ps
      and "custom_map_x=[string][int]$customX.Value" in ps
      and "custom_map_y=[string][int]$customY.Value" in ps)
check("exact native regular rules and random-leader flags represented",
      ps.count("Add-RuleChoice '") == 16
      and "New-Tab 'Game rules & victory'" in ps
      and "Random faction leader personalities" in ps
      and "Random faction leader social agendas" in ps
      and "Unity Survey available at start' 0x0100" in ps
      and "if ($entry.Inverted) { $selected = -not $selected }" in ps
      and 'autosim_profile_int(value, 65535, &p->rules_mask)' in main
      and 'autosim_profile_int(value, 3, &p->rules_random)' in main)
rule_values = {
    name: int(value, 16)
    for name, value in re.findall(
        r"\b(RULES_(?:VICTORY_\w+|DO_OR_DIE|LOOK_FIRST|TECH_STAGNATION|"
        r"INTENSE_RIVALRY|TIME_WARP|NO_UNITY_SURVEY|BLIND_RESEARCH|"
        r"IRONMAN|NO_UNITY_SCATTERING|SPOILS_OF_WAR|BELL_CURVE))\s*=\s*(0x[0-9A-Fa-f]+)",
        enums,
    )
}
check("all 16 available normal-rule enum bits are in UI",
      len(rule_values) == 16
      and sorted(rule_values.values()) == sorted(
          int(x, 16) for x in re.findall(
              r"Add-RuleChoice '[^']+' (0x[0-9A-Fa-f]+)", ps
          )
      ))
check("native new-game flags applied once before map setup and never persisted",
      "if (autosim_new_rules_override) {" in game
      and "AlphaIniPrefs->rules = (AlphaIniPrefs->rules & 0xFFFF0000u)" in game
      and "STATE_RAND_FAC_LEADER_PERSONALITIES" in game
      and "STATE_RAND_FAC_LEADER_SOCIAL_AGENDA" in game
      and game.index("if (autosim_new_rules_override)") < game.index("if (map_menu(1)")
      and "if (!autosim_auto_new_game) prefs_save(0);" in game)
check("old run profiles and loaded SAVs remain unaffected",
      "int rules_mode = 0;" in main
      and "int world_source = 0;" in main
      and 'autosim_auto_new_game = p.mode == "new";' in main
      and "if (autosim_auto_new_game) {" in main
      and "$rulesTab.Enabled=$newGame.Checked" in ps
      and "$worldTab.Enabled=$newGame.Checked" in ps)
check("native game’s difficult/scenario rule bits not overridden",
      "AlphaIniPrefs->rules & 0xFFFF0000u" in game
      and "auto_turn_eligible" not in game[game.index("if (autosim_new_rules_override)"):game.index("if (map_menu(1)")])

print("AUTO-LAUNCH custom map/game rules source contracts passed; native Windows test pending")
