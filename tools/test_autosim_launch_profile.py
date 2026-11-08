#!/usr/bin/env python3
"""Stage A source contract; the PowerShell writer is also self-tested in CI."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
bat = (r / "Run Automation.bat").read_text()
ps = (r / "Run Automation.ps1").read_text()
cpp = (r / "src/main.cpp").read_text()
hdr = (r / "src/main.h").read_text()
ui = (r / "src/gui_dialog.cpp").read_text()
turn = (r / "src/gameturn.cpp").read_text()
control = (r / "src/autosim_control.cpp").read_text()
game = (r / "src/game.cpp").read_text()
tele = (r / "src/telemetry.cpp").read_text()
workflow = (r / ".github/workflows/build-windows.yml").read_text()
checks = {
    "one batch/PowerShell launcher, no alternate game executable":
        "powershell.exe" in bat and "Run Automation.ps1" in bat and
        "New-Object System.Diagnostics.ProcessStartInfo" in ps,
    "pre-game WinForms GUI, existing SAV selection and turn cap":
        all(s in ps for s in ("System.Windows.Forms.Form",
        "System.Windows.Forms.OpenFileDialog", "System.Windows.Forms.NumericUpDown",
        "New game (native setup; manual AI handover)", "Load existing .sav")),
    "persistent defaults separate from immutable run-profile snapshots":
        all(s in ps for s in ("automation.ini", "automation\\profiles",
        "New-RunProfile", "Save-Options", "Read-Options", "schema=1")),
    "profile passed only in child process environment":
        "EnvironmentVariables['THINKERNECA_AUTOSIM_PROFILE']" in ps and
        "UseShellExecute = $false" in ps,
    "DLL profile read at startup, in separate automation section":
        'GetEnvironmentVariableA(' in cpp and
        'ini_parse(path, autosim_profile_option, &p)' in cpp and
        'strcmp(section, "automation")' in cpp,
    "normal mode disables all legacy automation keys":
        all(f"cf->{k} = 0;" in cpp for k in
        ("telemetry_enabled", "autosim_manual_turns",
         "autosim_suppress_notices", "autosim_fast_combat")),
    "simulation guard requires launcher opt-in":
        "autosim_launcher_active && auto_turn_eligible" in ui and
        "autosim_launcher_active && auto_turn_eligible" in turn and
        "return autosim_launcher_active" in turn and
        "!autosim_launcher_active || *GameHalted" in control,
    "explicit loaded SAV failure does not silently create a game":
        "else if (autosim_launcher_active)" in game and
        "The simulation will not start." in game,
    "campaign name and snapshot fingerprint in recorder":
        "autosim_launch_campaign_name" in hdr and
        "autosim_launch_campaign_name" in tele and
        "file_fingerprint(autosim_launch_profile_path.c_str())" in tele,
    "saved campaign preferences do not overwrite launch choices":
        "if (linked_turns > 0 && !autosim_launcher_active)" in tele,
    "launcher scripts included in Develop build artifact":
        "Run Automation.bat" in workflow and
        "Run Automation.ps1" in workflow and "pwsh -NoProfile" in workflow,
}
for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)
assert all(checks.values())
