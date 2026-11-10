param([switch]$SelfTest, [switch]$GuiSelfTest, [switch]$ReportSelfTest)

# Stage A launch GUI. Nothing in thinker.ini, thinker_user.ini or the game's
# Alpha Centauri.ini is written. The same thinker.exe is always launched.
$ErrorActionPreference = 'Stop'
$gameRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $MyInvocation.MyCommand.Path))
$prefsPath = Join-Path $gameRoot 'automation.ini'

# Independent launcher log is created BEFORE starting the game.
# It remains available even if terranx crashes before telemetry starts.
$script:launcherLogPath = $null
$script:launcherTranscriptStarted = $false
if (-not ($SelfTest -or $GuiSelfTest -or $ReportSelfTest)) {
    try {
        $logs = Join-Path $gameRoot 'automation\logs'
        [void][System.IO.Directory]::CreateDirectory($logs)
        $name = 'launcher-' + (Get-Date -Format 'yyyyMMdd-HHmmss') +
                '-' + [Guid]::NewGuid().ToString('N').Substring(0,8) + '.log'
        $script:launcherLogPath = Join-Path $logs $name
        Start-Transcript -Path $script:launcherLogPath -ErrorAction Stop | Out-Null
        $script:launcherTranscriptStarted = $true
    } catch {
        Write-Host ('WARNING: Launcher diagnostics unavailable: ' + $_.Exception.Message)
    }
    Write-Host 'thinkerNeca Automation Launcher'
    Write-Host ('Game directory: ' + $gameRoot)
    if ($script:launcherLogPath) {
        Write-Host ('Diagnostic log: ' + $script:launcherLogPath)
    }
}

function New-Defaults {
    return @{
        mode = 'new'
        source_save = ''
        campaign_name = ('AI Simulation ' + (Get-Date -Format 'yyyy-MM-dd HHmm'))
        turns = '100'
        telemetry_enabled = '1'
        suppress_notices = '1'
        fast_combat = '1'
        unlimited_expansion = '1'
        timing_enabled = '1'
        archive_interval = '25'
        faction_1 = 'GAIANS'
        faction_2 = 'HIVE'
        faction_3 = 'UNIV'
        faction_4 = 'MORGAN'
        faction_5 = 'SPARTANS'
        faction_6 = 'BELIEVE'
        faction_7 = 'PEACE'
        render_mode = '2'
        world_source = '0'
        world_size = '3'
        custom_map_x = '80'
        custom_map_y = '160'
        rules_mode = '0' # legacy installed-game preferences until explicitly customised
        rules_mask = '6670' # stock default 0x1A0E
        rules_random = '0'
        world_ocean = '1'
        world_erosion = '1'
        world_cloud = '1'
        world_life = '1'
        difficulty = '3'
        observer = '1'
        monitor_seconds = '45'
    }
}

function Read-Options([string]$path) {
    $values = New-Defaults
    if (-not [System.IO.File]::Exists($path)) { return $values }
    $section = ''
    foreach ($raw in [System.IO.File]::ReadAllLines($path, [System.Text.Encoding]::Default)) {
        $line = $raw.Trim()
        if ($line.Length -eq 0 -or $line.StartsWith(';') -or $line.StartsWith('#')) { continue }
        if ($line.StartsWith('[') -and $line.EndsWith(']')) {
            $section = $line.Substring(1, $line.Length - 2)
            continue
        }
        if ($section -ne 'automation') { continue }
        $split = $line.IndexOf('=')
        if ($split -lt 1) { continue }
        $key = $line.Substring(0, $split).Trim()
        if ($values.ContainsKey($key)) { $values[$key] = $line.Substring($split + 1).Trim() }
    }
    return $values
}

function New-OptionLines($options, [bool]$includeSchema) {
    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add('[automation]')
    if ($includeSchema) { $lines.Add('schema=1') }
    foreach ($key in @('mode', 'source_save', 'campaign_name',
                      'turns', 'telemetry_enabled', 'suppress_notices', 'fast_combat',
                      'unlimited_expansion', 'timing_enabled', 'archive_interval',
                      'faction_1', 'faction_2', 'faction_3', 'faction_4',
                      'faction_5', 'faction_6', 'faction_7',
                      'render_mode', 'world_source', 'world_size',
                      'custom_map_x', 'custom_map_y', 'rules_mode',
                      'rules_mask', 'rules_random', 'world_ocean', 'world_erosion',
                      'world_cloud', 'world_life', 'difficulty', 'observer',
                      'monitor_seconds')) {
        $lines.Add($key + '=' + [string]$options[$key])
    }
    return $lines.ToArray()
}

function Save-Options([string]$path, $options, [bool]$includeSchema) {
    $lines = New-OptionLines $options $includeSchema
    # The per-run snapshot is immutable after creation. The regular settings
    # file only provides GUI defaults; it is never used directly by the DLL.
    [System.IO.File]::WriteAllLines($path, [string[]]$lines,
                                   [System.Text.Encoding]::Default)
}

function New-RunProfile([string]$root, $options) {
    $dir = Join-Path $root 'automation\profiles'
    [void][System.IO.Directory]::CreateDirectory($dir)
    $name = (Get-Date -Format 'yyyyMMddTHHmmss') + '-' +
            [Guid]::NewGuid().ToString('N').Substring(0, 8) + '.ini'
    $path = Join-Path $dir $name
    Save-Options $path $options $true
    return $path
}

if ($SelfTest) {
    $root = Join-Path ([System.IO.Path]::GetTempPath()) ('thinkerneca-auto-test-' +
        [Guid]::NewGuid().ToString('N'))
    [void][System.IO.Directory]::CreateDirectory($root)
    try {
        $options = New-Defaults
        $options['campaign_name'] = 'Path and profile smoke'
        $options['mode'] = 'load'
        $options['source_save'] = 'C:\Sample Games\Autosave_2200.sav'
        $prefs = Join-Path $root 'automation.ini'
        Save-Options $prefs $options $false
        $again = Read-Options $prefs
        if ($again['campaign_name'] -ne $options['campaign_name'] -or
            $again['source_save'] -ne $options['source_save']) {
            throw 'settings roundtrip failed'
        }
        $snapshot = New-RunProfile $root $again
        $lines = [System.IO.File]::ReadAllText($snapshot)
        if (-not $lines.Contains('[automation]') -or
            -not $lines.Contains('schema=1') -or
            -not $lines.Contains('mode=load') -or
            -not $lines.Contains('turns=100') -or
            -not $lines.Contains('render_mode=2') -or
            -not $lines.Contains('unlimited_expansion=1') -or
            -not $lines.Contains('timing_enabled=1') -or
            -not $lines.Contains('archive_interval=25') -or
            -not $lines.Contains('faction_7=PEACE') -or
            -not $lines.Contains('world_source=0') -or
            -not $lines.Contains('world_size=3') -or
            -not $lines.Contains('custom_map_x=80') -or
            -not $lines.Contains('custom_map_y=160') -or
            -not $lines.Contains('rules_mode=0') -or
            -not $lines.Contains('rules_mask=6670') -or
            -not $lines.Contains('rules_random=0') -or
            -not $lines.Contains('monitor_seconds=45')) {
            throw 'run-profile snapshot mismatch'
        }
        $again['mode'] = 'new'
        $again['source_save'] = ''
        $again['world_source'] = '2'
        $again['world_size'] = '5'
        $again['custom_map_x'] = '99'
        $again['custom_map_y'] = '180'
        $again['rules_mode'] = '1'
        $again['rules_mask'] = '513'
        $again['rules_random'] = '3'
        $newWorldProfile = New-RunProfile $root $again
        $newWorldLines = [System.IO.File]::ReadAllText($newWorldProfile)
        if (-not $newWorldLines.Contains('mode=new') -or
            -not $newWorldLines.Contains('world_source=2') -or
            -not $newWorldLines.Contains('world_size=5') -or
            -not $newWorldLines.Contains('custom_map_x=99') -or
            -not $newWorldLines.Contains('custom_map_y=180') -or
            -not $newWorldLines.Contains('rules_mode=1') -or
            -not $newWorldLines.Contains('rules_mask=513') -or
            -not $newWorldLines.Contains('rules_random=3')) {
            throw 'custom world/rules snapshot mismatch'
        }
        Write-Output 'AUTO-LAUNCH Stage A PowerShell profile smoke passed'
    } finally {
        [System.IO.Directory]::Delete($root, $true)
    }
    exit 0
}


# Workstation monitor: warning only, never automatically kill the game.
# The DLL writes <profile>.status atomically at faction/phase boundaries and
# reads <profile>.command on those same boundaries.
function Write-RunCommand([string]$runProfilePath, [string]$command) {
    $destination = $runProfilePath + '.command'
    [System.IO.File]::WriteAllText($destination, $command + [Environment]::NewLine,
                                  [System.Text.Encoding]::ASCII)
}
function Show-GameWindow([int]$pidToShow) {
    try {
        $process = [System.Diagnostics.Process]::GetProcessById($pidToShow)
        $process.Refresh()
        $handle = $process.MainWindowHandle
        if ($handle -eq [IntPtr]::Zero) { Write-Host 'No game window available yet.'; return $null }
        [AutoMonitorWin32]::ShowWindow($handle, 9) | Out-Null
        [AutoMonitorWin32]::SetForegroundWindow($handle) | Out-Null
        return $handle
    } catch {
        Write-Host ('Show Game failed: ' + $_.Exception.Message)
        return $null
    }
}
function Save-GameScreenshot([int]$pidToCapture, [string]$root) {
    $window = Show-GameWindow $pidToCapture
    if ($null -eq $window) { return }
    $rect = New-Object AutoMonitorRect
    if (-not [AutoMonitorWin32]::GetWindowRect($window, [ref]$rect)) {
        Write-Host 'Could not determine game window bounds.'; return
    }
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -lt 20 -or $height -lt 20) { Write-Host 'Invalid window bounds.'; return }
    $dir = Join-Path $root 'automation\screenshots'
    [void][System.IO.Directory]::CreateDirectory($dir)
    $filename = Join-Path $dir ((Get-Date -Format 'yyyyMMdd_HHmmss') + '_game.png')
    $bitmap = New-Object System.Drawing.Bitmap $width,$height
    try {
        $drawing = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $drawing.CopyFromScreen($rect.Left, $rect.Top, 0, 0,
                (New-Object System.Drawing.Size($width,$height)))
        } finally { $drawing.Dispose() }
        $bitmap.Save($filename,[System.Drawing.Imaging.ImageFormat]::Png)
        Write-Host ('Screenshot saved: ' + $filename)
        Write-Host 'If Null Rendering was active, only previously drawn content may be visible.'
    } finally { $bitmap.Dispose() }
}
function Run-Monitor([string]$runProfilePath, [int]$warningSeconds, [int]$launcherPid) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct AutoMonitorRect { public int Left, Top, Right, Bottom; }
public static class AutoMonitorWin32 {
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hwnd, int show);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hwnd, ref AutoMonitorRect rect);
    [DllImport("kernel32.dll")] public static extern IntPtr GetConsoleWindow();
}
'@
    $statusFile = $runProfilePath + '.status'
    $pidFile = $runProfilePath + '.pid'
    $lastRecord = ''
    $lastProgress = [DateTime]::UtcNow
    $gamePid = 0
    $reportedStall = $false
    $monitorStart = [DateTime]::UtcNow
    $consoleFocusAttempted = $false
    Write-Host ''
    Write-Host 'thinkerNeca Automation Monitor'
    Write-Host ('Run profile: ' + $runProfilePath)
    Write-Host ('Game PID handoff: ' + $pidFile)
    Write-Host ('Game progress: ' + $statusFile)
    Write-Host 'The monitor will try to take focus when AI processing begins.'
    Write-Host 'G: Show game  |  R: Restore rendering  |  N: Re-enable Null Rendering (NOT AI pause)'
    Write-Host 'P: Screenshot  |  S: Stop at next full turn  |  Space: Pause AI at next full turn'
    Write-Host 'Q: Leave monitor open? No - Q closes this console WITHOUT killing the game.'
    Write-Host ('No-progress warning: ' + $warningSeconds + ' seconds. Never auto-kills the game.')
    while ($true) {
        if ($gamePid -eq 0 -and [System.IO.File]::Exists($pidFile)) {
            $foundPid = 0
            if ([int]::TryParse([System.IO.File]::ReadAllText($pidFile).Trim(),
                    [ref]$foundPid) -and $foundPid -gt 0) {
                $gamePid = $foundPid
                Write-Host ('Game process detected: PID ' + $gamePid)
            }
        }
        if ($gamePid -eq 0 -and
            ([DateTime]::UtcNow - $monitorStart).TotalSeconds -gt 3) {
            $launcherAlive = $false
            try {
                $launcherProcess = [System.Diagnostics.Process]::GetProcessById($launcherPid)
                $launcherAlive = -not $launcherProcess.HasExited
            } catch { $launcherAlive = $false }
            if (-not $launcherAlive) {
                throw ('thinker.exe exited without creating a terranx.exe PID file. ' +
                    'Check the launcher log and debug.txt for a startup/injection error.')
            }
        }
        if ([System.IO.File]::Exists($statusFile)) {
            try {
                $record = @{}
                foreach ($line in [System.IO.File]::ReadAllLines($statusFile)) {
                    $split = $line.IndexOf('=')
                    if ($split -gt 0) {
                        $record[$line.Substring(0,$split)] = $line.Substring($split+1)
                    }
                }
                if ($record.ContainsKey('pid') -and
                    [int]::TryParse($record['pid'], [ref]$gamePid)) {
                    # The process is intentionally discovered from the game DLL,
                    # not from thinker.exe, which exits immediately after launch.
                }
                $signature = @($record['turn'],$record['year'],
                               $record['faction'],$record['phase'],$record['render']) -join '/'
                if ($signature -ne $lastRecord) {
                    Write-Host ('MY ' + $record['year'] +
                        ' | turn ' + $record['turn'] +
                        ' | faction ' + $record['faction'] +
                        ' | ' + $record['phase'] +
                        ' | ' + $record['render'] +
                        ' (configured=' + $record['configured_render'] +
                        ', draws skipped=' + $record['draw_skipped'] +
                        ', presents skipped=' + $record['screen_skipped'] + ')')
                    if ($record['phase'] -eq 'victory_paused') {
                        Write-Host 'Victory reached. Use the game window to Save, Continue, Take Control or Exit.'
                    }
                    $lastRecord = $signature
                    # SMAC takes foreground focus on launch, making the
                    # external console keyboard controls hard to reach.
                    # Attempt foreground only ONCE on the first live AI phase.
                    if (-not $consoleFocusAttempted -and $record['phase'] -eq 'ai_turn') {
                        $consoleFocusAttempted = $true
                        $consoleWindow = [AutoMonitorWin32]::GetConsoleWindow()
                        if ($consoleWindow -ne [IntPtr]::Zero) {
                            [AutoMonitorWin32]::ShowWindow($consoleWindow, 9) | Out-Null
                            if ([AutoMonitorWin32]::SetForegroundWindow($consoleWindow)) {
                                Write-Host 'Monitor console focused; R/N/S/Space/Q commands ready.'
                            } else {
                                Write-Host 'Windows declined console focus. Alt+Tab to use monitor keys.'
                            }
                        } else {
                            Write-Host 'No console window handle available. Alt+Tab to the monitor.'
                        }
                    }
                    $lastProgress = [DateTime]::UtcNow
                    $reportedStall = $false
                }
                if ($record['phase'] -eq 'turn_driver_exit') {
                    Write-Host 'Simulation driver exited. Check game and campaign logs.'
                    return
                }
            } catch {
                Write-Host ('Monitor read error: ' + $_.Exception.Message)
            }
        }
        if ($gamePid -gt 0) {
            try {
                $game = [System.Diagnostics.Process]::GetProcessById($gamePid)
                if ($game.HasExited) {
                    Write-Host ('Game process exited with code ' + $game.ExitCode)
                    return
                }
            } catch {
                Write-Host 'Game process has exited or is no longer accessible.'
                return
            }
        }
        $now = [DateTime]::UtcNow
        $elapsed = ($now - $lastProgress).TotalSeconds
        if ($elapsed -ge $warningSeconds -and -not $reportedStall -and
            $record['phase'] -ne 'victory_paused' -and
            $record['phase'] -ne 'operator_dialog') {
            Write-Warning ('No phase/turn progress for ' + [int]$elapsed +
                ' seconds. Possible lengthy AI turn, modal, or hang; game not terminated.')
            try { [Console]::Beep(900,200) } catch {}
            $reportedStall = $true
        }
        if ([Console]::KeyAvailable) {
            $key = [Console]::ReadKey($true)
            switch ($key.Key) {
                'G' { if ($gamePid -gt 0) { Show-GameWindow $gamePid | Out-Null } }
                'R' {
                    Write-RunCommand $runProfilePath 'SHOW'
                    Write-Host 'Restore rendering queued; applied at next live game phase.'
                    if ($gamePid -gt 0) { Show-GameWindow $gamePid | Out-Null }
                }
                'N' {
                    Write-RunCommand $runProfilePath 'NULL'
                    Write-Host 'Null Rendering queued for next live phase; N does not pause AI. Use Space.'
                }
                'P' { if ($gamePid -gt 0) { Save-GameScreenshot $gamePid $gameRoot } }
                'S' {
                    Write-RunCommand $runProfilePath 'STOP'
                    Write-Host 'Stop queued for next completed full turn.'
                }
                'Spacebar' {
                    Write-RunCommand $runProfilePath 'PAUSE'
                    Write-Host 'Pause queued for next completed full turn.'
                }
                'Q' { Write-Host 'Monitor closed; game continues.'; return }
            }
        }
        Start-Sleep -Milliseconds 350
    }
}

# Report calculations remain in the tested Python standard-library module.
# The PowerShell launcher owns the complete user interface; no second GUI,
# Tkinter installation or additional Windows process window is required.
function Resolve-CampaignReporterPython {
    foreach ($candidate in @('python.exe', 'py.exe')) {
        $command = Get-Command $candidate -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($command) {
            return @{
                FileName = [string]$command.Source
                Prefix = if ($candidate -eq 'py.exe') {'-3'} else {''}
            }
        }
    }
    throw 'Python 3 is required for campaign reports. Install Python 3 and select Refresh.'
}

function Quote-CampaignReporterArg([string]$value) {
    # File system names on Windows cannot contain a quote character.
    if ($value.Contains('"') -or $value.IndexOf([char]13) -ge 0 -or
        $value.IndexOf([char]10) -ge 0) {
        throw 'Invalid argument to campaign reporter.'
    }
    # A final backslash would escape the closing quote in Windows argv.
    if ($value.EndsWith('\')) { $value += '.' }
    return '"' + $value + '"'
}

function Invoke-CampaignReporter([string[]]$arguments) {
    $scriptPath = Join-Path $gameRoot 'tools\campaign_report.py'
    if (-not [System.IO.File]::Exists($scriptPath)) {
        throw 'Missing tools\campaign_report.py beside the installed automation launcher.'
    }
    $python = Resolve-CampaignReporterPython
    $argv = New-Object 'System.Collections.Generic.List[string]'
    if ($python['Prefix']) { $argv.Add([string]$python['Prefix']) }
    $argv.Add($scriptPath)
    foreach ($argument in $arguments) { $argv.Add([string]$argument) }
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = [string]$python['FileName']
    $startInfo.WorkingDirectory = $gameRoot
    $startInfo.Arguments = ((@($argv) | ForEach-Object {
        Quote-CampaignReporterArg $_
    }) -join ' ')
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true
    $process = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $process) { throw 'Could not start Python campaign reporter.' }
    try {
        $result = $process.StandardOutput.ReadToEnd()
        $errors = $process.StandardError.ReadToEnd()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) {
            throw ('Campaign reporter failed: ' + $errors.Trim())
        }
        return $result.Trim()
    } finally {
        $process.Dispose()
    }
}

# Convert the JSON ARRAY to individual session objects, irrespective of
# ConvertFrom-Json's PowerShell 5.1 single/array pipeline behaviour. A nested
# array must never become one ListViewItem with joined campaign names.
function Add-CampaignSessionNode($value, [System.Collections.Generic.List[object]]$rows) {
    if ($null -eq $value) { return }
    if ($value -is [System.Array]) {
        foreach ($item in $value) { Add-CampaignSessionNode $item $rows }
        return
    }
    if ($null -eq $value.PSObject.Properties['campaign_dir'] -or
        $null -eq $value.PSObject.Properties['session_id']) {
        throw 'Malformed campaign scan row: missing campaign_dir or session_id.'
    }
    [void]$rows.Add($value)
}

function Convert-CampaignSessionRows([string]$json) {
    if ([string]::IsNullOrWhiteSpace($json)) {
        throw 'Campaign scanner returned no JSON.'
    }
    $decoded = ConvertFrom-Json -InputObject $json
    $rows = New-Object 'System.Collections.Generic.List[object]'
    Add-CampaignSessionNode $decoded $rows
    # Preserve array identity even when one or zero rows are present.
    return ,($rows.ToArray())
}

if ($ReportSelfTest) {
    # Exercise the actual CLI bridge with one synthetic, zero-turn session.
    # This test does not need thinker.exe, a game directory or Tkinter.
    $tmp = Join-Path ([System.IO.Path]::GetTempPath()) ('thinkerneca-report-test-' +
        [Guid]::NewGuid().ToString('N'))
    $session = Join-Path $tmp 'campaign-test\sessions\session-001'
    [void][System.IO.Directory]::CreateDirectory($session)
    try {
        [System.IO.File]::WriteAllText((Join-Path $session 'manifest.json'),
            '{"schema":1,"started_utc":"20261109T000000Z","source_save":null}')
        [System.IO.File]::WriteAllText((Join-Path $session 'status.json'),
            '{"schema":1,"state":"closed","last_committed_turn":-1,"updated_utc":"20261109T000000Z"}')
        [System.IO.File]::WriteAllText((Join-Path $session 'turns.jsonl'), '')
        $json = Invoke-CampaignReporter -arguments @('--scan-json', $tmp)
        $rows = Convert-CampaignSessionRows $json
        if ($rows.Count -ne 1 -or $rows[0].session_id -ne 'session-001' -or
            $rows[0].status -ne 'Closed') {
            throw 'Headless campaign/session scan contract failed.'
        }
        # The previous one-session test could not detect a JSON array being
        # displayed as a single row. Exercise multiple distinct games and
        # multiple sessions of one campaign through the actual scan bridge.
        $other = Join-Path $tmp 'campaign-second\sessions\session-001'
        $continuation = Join-Path $tmp 'campaign-test\sessions\session-002'
        foreach ($directory in @($other, $continuation)) {
            [void][System.IO.Directory]::CreateDirectory($directory)
            [System.IO.File]::WriteAllText((Join-Path $directory 'manifest.json'),
                '{"schema":1,"started_utc":"20261109T000000Z","source_save":null}')
            [System.IO.File]::WriteAllText((Join-Path $directory 'status.json'),
                '{"schema":1,"state":"closed","last_committed_turn":-1,"updated_utc":"20261109T000000Z"}')
            [System.IO.File]::WriteAllText((Join-Path $directory 'turns.jsonl'), '')
        }
        $json = Invoke-CampaignReporter -arguments @('--scan-json', $tmp)
        $rows = Convert-CampaignSessionRows $json
        if ($rows.Count -ne 3) {
            throw ('Multiple campaign sessions were collapsed: got ' + $rows.Count)
        }
        $identities = @($rows | ForEach-Object {
            [string]$_.campaign_dir + '\' + [string]$_.session_id
        } | Sort-Object -Unique)
        if ($identities.Count -ne 3) {
            throw 'Campaign/session identities were lost during JSON conversion.'
        }
        $emptyRows = Convert-CampaignSessionRows '[]'
        if ($emptyRows.Count -ne 0) { throw 'Empty campaign list must remain empty.' }
        Write-Host 'AUTO-LAUNCH multi-campaign/session report bridge passed'
    } finally {
        if ([System.IO.Directory]::Exists($tmp)) {
            [System.IO.Directory]::Delete($tmp, $true)
        }
    }
    exit 0
}

try {
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    [System.Windows.Forms.Application]::EnableVisualStyles()
    if (-not $GuiSelfTest) {
        foreach ($filename in @('thinker.exe','thinker.dll','terranx.exe')) {
            if (-not [System.IO.File]::Exists((Join-Path $gameRoot $filename))) {
                throw ('Missing ' + $filename + ' beside Run Automation.bat')
            }
        }
    }
    $options = Read-Options $prefsPath
    $form = New-Object System.Windows.Forms.Form
    $form.Text = 'thinkerNeca - Automation Launcher'
    $form.Size = New-Object System.Drawing.Size(1035,890)
    $form.MinimumSize = $form.MaximumSize = $form.Size
    $form.StartPosition = 'CenterScreen'
    $form.FormBorderStyle = 'FixedDialog'
    $form.MaximizeBox = $false
    $form.AutoScaleMode = 'Font'

    $tabs = New-Object System.Windows.Forms.TabControl
    $tabs.SetBounds(22,22,975,737)
    $form.Controls.Add($tabs)
    function New-Tab([string]$name) {
        $tab = New-Object System.Windows.Forms.TabPage
        $tab.Text = $name
        $tab.AutoScroll = $true
        $tabs.TabPages.Add($tab)
        return $tab
    }
    $runTab = New-Tab 'Game & campaign'
    $worldTab = New-Tab 'World generation'
    $rulesTab = New-Tab 'Game rules & victory'
    $factionTab = New-Tab 'Faction roster'
    $presentationTab = New-Tab 'Presentation & logs'
    $reportTab = New-Tab 'Campaign tools & archive'
    $monitorTab = New-Tab 'Monitor & recovery'
    function Add-Label($tab, [string]$text, [int]$x, [int]$y, [int]$w, [int]$h) {
        $label = New-Object System.Windows.Forms.Label
        $label.Text = $text
        $label.SetBounds($x,$y,$w,$h)
        $tab.Controls.Add($label)
        return $label
    }
    function Add-Text($tab,[string]$initial,[int]$x,[int]$y,[int]$w) {
        $control = New-Object System.Windows.Forms.TextBox
        $control.Text = $initial
        $control.SetBounds($x,$y,$w,28)
        $tab.Controls.Add($control)
        return $control
    }
    function Add-Check($tab,[string]$text,[int]$y,[bool]$checked) {
        $control = New-Object System.Windows.Forms.CheckBox
        $control.Text = $text
        $control.SetBounds(25,$y,865,32)
        $control.Checked = $checked
        $tab.Controls.Add($control)
        return $control
    }
    function Add-Combo($tab,[string]$label,[int]$y,[string[]]$values,[int]$selected) {
        [void](Add-Label $tab $label 28 $y 880 24)
        $control = New-Object System.Windows.Forms.ComboBox
        $control.DropDownStyle = 'DropDownList'
        $control.SetBounds(28,($y+30),420,30)
        [void]$control.Items.AddRange([object[]]$values)
        $control.SelectedIndex = [Math]::Max(0,[Math]::Min($values.Length-1,$selected))
        $tab.Controls.Add($control)
        return $control
    }
    function Get-Selected([string]$name,[int]$maximum) {
        $value = 0
        if ([int]::TryParse([string]$options[$name],[ref]$value)) {
            return [Math]::Max(0,[Math]::Min($maximum,$value))
        }
        return 0
    }

    [void](Add-Label $runTab 'Run source' 27 20 850 28)
    $newGame = New-Object System.Windows.Forms.RadioButton
    $newGame.Text = 'New game - setup from automation profile'
    $newGame.SetBounds(28,62,800,32)
    $runTab.Controls.Add($newGame)
    $loadGame = New-Object System.Windows.Forms.RadioButton
    $loadGame.Text = 'Load an existing SAV'
    $loadGame.SetBounds(28,103,800,32)
    $runTab.Controls.Add($loadGame)
    [void](Add-Label $runTab 'Existing .sav file' 28 151 850 25)
    $source = Add-Text $runTab ([string]$options['source_save']) 28 180 750
    $browse = New-Object System.Windows.Forms.Button
    $browse.Text = 'Browse...'
    $browse.SetBounds(795,179,120,32)
    $runTab.Controls.Add($browse)
    [void](Add-Label $runTab 'Campaign display name' 28 247 850 25)
    $campaign = Add-Text $runTab ([string]$options['campaign_name']) 28 278 880
    $campaign.MaxLength = 88
    [void](Add-Label $runTab 'Completed AI turns before operator menu' 28 350 850 25)
    $turns = New-Object System.Windows.Forms.NumericUpDown
    $turns.Minimum = 1; $turns.Maximum = 100000
    $turns.SetBounds(28,382,180,32)
    $n=100
    if ([int]::TryParse([string]$options['turns'],[ref]$n) -and $n -ge 1 -and $n -le 100000) {
        $turns.Value = [decimal]$n
    }
    $runTab.Controls.Add($turns)
    [void](Add-Label $runTab 'No regular INI files are edited. SAVs still use existing folders (Stage B later).' 28 480 880 50)
    [void](Add-Label $runTab 'New-game engine automation is experimental; normal thinker.exe remains unchanged.' 28 535 880 50)

    [void](Add-Label $worldTab 'New-game world. Fixed Planet maps use original game map files; climate settings only affect generated worlds.' 28 10 912 38)
    function Add-WorldSelect([string]$label,[int]$x,[int]$y,[int]$w,[string[]]$choices,[int]$selected) {
        [void](Add-Label $worldTab $label $x $y $w 23)
        $combo = New-Object System.Windows.Forms.ComboBox
        $combo.DropDownStyle = 'DropDownList'
        $combo.SetBounds($x,($y+26),$w,30)
        [void]$combo.Items.AddRange([object[]]$choices)
        $combo.SelectedIndex = [Math]::Max(0,[Math]::Min($choices.Length-1,$selected))
        $worldTab.Controls.Add($combo)
        return $combo
    }
    $difficulty = Add-WorldSelect 'Difficulty' 28 58 280 @(
        'Citizen','Specialist','Talent','Librarian','Thinker','Transcend') (Get-Selected 'difficulty' 5)
    $observer = Add-WorldSelect 'Initial observer (not a human faction)' 365 58 320 @(
        'Faction slot 1','Faction slot 2','Faction slot 3','Faction slot 4',
        'Faction slot 5','Faction slot 6','Faction slot 7') ([Math]::Max(0,(Get-Selected 'observer' 7)-1))
    $worldSource = Add-WorldSelect 'Map source' 28 143 820 @(
        'Generated Planet (random terrain + selected climate)',
        'Map of Planet (maps\xplanet.mp)',
        'Huge Map of Planet (maps\xplanetx.mp)') (Get-Selected 'world_source' 2)
    $worldSize = Add-WorldSelect 'Generated Planet size' 28 229 340 @(
        'Tiny (24 x 48)','Small (32 x 64)','Standard (40 x 80)',
        'Large (50 x 100)','Huge (64 x 128)','Very Huge (80 x 160)',
        'Custom dimensions') (Get-Selected 'world_size' 6)
    [void](Add-Label $worldTab 'Custom X (native half-width)' 410 229 205 24)
    [void](Add-Label $worldTab 'Custom Y (height)' 644 229 193 24)
    $customX = New-Object System.Windows.Forms.NumericUpDown
    $customY = New-Object System.Windows.Forms.NumericUpDown
    foreach ($control in @($customX,$customY)) {
        $control.Minimum = 16
        $control.Maximum = 512
        $control.SetBounds($(if ($control -eq $customX) {410} else {644}),255,160,30)
        $worldTab.Controls.Add($control)
    }
    $initialX = 80
    if (![int]::TryParse([string]$options['custom_map_x'],[ref]$initialX)) { $initialX = 80 }
    $initialY = 160
    if (![int]::TryParse([string]$options['custom_map_y'],[ref]$initialY)) { $initialY = 160 }
    $customX.Value = [decimal][Math]::Min(512,[Math]::Max(16,$initialX))
    $customY.Value = [decimal][Math]::Min(512,[Math]::Max(16,$initialY))
    [void](Add-Label $worldTab 'Native Custom Map X is doubled internally (MapAreaX = 2 * X). Values 16–512; large dimensions are experimental.' 28 310 895 40)

    $worldOcean = Add-WorldSelect 'Ocean coverage' 28 371 275 @(
        'Low','Average','High') (Get-Selected 'world_ocean' 2)
    $worldErosion = Add-WorldSelect 'Erosion' 365 371 275 @(
        'Low','Average','High') (Get-Selected 'world_erosion' 2)
    $worldCloud = Add-WorldSelect 'Cloud cover' 28 462 275 @(
        'Low','Average','High') (Get-Selected 'world_cloud' 2)
    $worldLife = Add-WorldSelect 'Native life' 365 462 275 @(
        'Low','Average','High') (Get-Selected 'world_life' 2)
    [void](Add-Label $worldTab 'A fixed Map of Planet ignores generated size, climate and Thinker random-map generation. Installed map files are required.' 28 573 890 60)
    $updateWorldSource = {
        $generated = $worldSource.SelectedIndex -eq 0
        $worldSize.Enabled = $generated
        $isCustom = $generated -and $worldSize.SelectedIndex -eq 6
        $customX.Enabled = $isCustom
        $customY.Enabled = $isCustom
        foreach ($setting in @($worldOcean,$worldErosion,$worldCloud,$worldLife)) {
            $setting.Enabled = $generated
        }
    }
    $worldSource.Add_SelectedIndexChanged($updateWorldSource)
    $worldSize.Add_SelectedIndexChanged($updateWorldSource)
    & $updateWorldSource

    # The native custom-game dialog has 15 ordinary bit choices, an inverted
    # Unity Survey choice, and two GameState leader-randomisation choices.
    # Preserve the exact enum bits instead of the native dialog's UI bit order.
    $customRules = New-Object System.Windows.Forms.CheckBox
    $customRules.Text = 'Use explicitly selected game rules (off = installed-game saved rules)'
    $customRules.SetBounds(28,18,865,34)
    $customRules.Checked = ([string]$options['rules_mode'] -eq '1')
    $rulesTab.Controls.Add($customRules)
    [void](Add-Label $rulesTab 'The checkboxes apply only to NEW games. They do not modify the game INI or a loaded save.' 28 55 886 29)
    [void](Add-Label $rulesTab 'Victory conditions' 28 91 850 28)
    $initialRuleMask = 6670 # 0x1A0E: the game native default
    if (![int]::TryParse([string]$options['rules_mask'],[ref]$initialRuleMask) -or
        $initialRuleMask -lt 0 -or $initialRuleMask -gt 65535) { $initialRuleMask = 6670 }
    $ruleControls = New-Object 'System.Collections.Generic.List[object]'
    function Add-RuleChoice([string]$label,[int]$bit,[int]$x,[int]$y,[bool]$inverted) {
        $cb = New-Object System.Windows.Forms.CheckBox
        $cb.Text = $label
        $cb.SetBounds($x,$y,432,32)
        $cb.Checked = ((($initialRuleMask -band $bit) -ne 0) -xor $inverted)
        $rulesTab.Controls.Add($cb)
        $ruleControls.Add([pscustomobject]@{Control=$cb; Bit=$bit; Inverted=$inverted})
    }
    Add-RuleChoice 'Transcendence victory' 0x0800 28 124 $false
    Add-RuleChoice 'Conquest victory' 0x0002 480 124 $false
    Add-RuleChoice 'Economic victory' 0x0004 28 163 $false
    Add-RuleChoice 'Diplomatic victory' 0x0008 480 163 $false
    Add-RuleChoice 'Cooperative victory enabled' 0x1000 28 202 $false

    [void](Add-Label $rulesTab 'Custom game rules' 28 249 850 27)
    Add-RuleChoice 'Do or Die (no escape from defeat)' 0x0001 28 284 $false
    Add-RuleChoice 'Look First (exploration preview)' 0x0010 480 284 $false
    Add-RuleChoice 'Tech Stagnation' 0x0020 28 323 $false
    Add-RuleChoice 'Spoils of War' 0x4000 480 323 $false
    Add-RuleChoice 'Blind Research' 0x0200 28 362 $false
    Add-RuleChoice 'Intense Rivalry' 0x0040 480 362 $false
    Add-RuleChoice 'Unity Survey available at start' 0x0100 28 401 $true
    Add-RuleChoice 'No Unity Pod scattering' 0x2000 480 401 $false
    Add-RuleChoice 'Bell Curve combat' 0x8000 28 440 $false
    Add-RuleChoice 'Time Warp' 0x0080 480 440 $false
    Add-RuleChoice 'Ironman' 0x0400 28 479 $false

    [void](Add-Label $rulesTab 'Faction-leader randomisation' 28 527 860 27)
    $randomPersonalities = New-Object System.Windows.Forms.CheckBox
    $randomPersonalities.Text = 'Random faction leader personalities'
    $randomPersonalities.SetBounds(28,559,410,32)
    $randomPersonalities.Checked = (([int]$options['rules_random'] -band 1) -ne 0)
    $rulesTab.Controls.Add($randomPersonalities)
    $randomAgendas = New-Object System.Windows.Forms.CheckBox
    $randomAgendas.Text = 'Random faction leader social agendas'
    $randomAgendas.SetBounds(480,559,423,32)
    $randomAgendas.Checked = (([int]$options['rules_random'] -band 2) -ne 0)
    $rulesTab.Controls.Add($randomAgendas)
    $ruleDefaults = New-Object System.Windows.Forms.Button
    $ruleDefaults.Text = 'Restore stock rules'
    $ruleDefaults.SetBounds(28,625,230,41)
    $rulesTab.Controls.Add($ruleDefaults)
    $ruleDefaults.Add_Click({
        foreach ($entry in $ruleControls) {
            $entry.Control.Checked = (((6670 -band $entry.Bit) -ne 0) -xor $entry.Inverted)
        }
        $randomPersonalities.Checked = $false
        $randomAgendas.Checked = $false
    })
    $updateRulesEnabled = {
        foreach ($entry in $ruleControls) { $entry.Control.Enabled = $customRules.Checked }
        $randomPersonalities.Enabled = $customRules.Checked
        $randomAgendas.Enabled = $customRules.Checked
        $ruleDefaults.Enabled = $customRules.Checked
    }
    $customRules.Add_CheckedChanged($updateRulesEnabled)
    & $updateRulesEnabled

    [void](Add-Label $factionTab 'Seven specific faction slots. Each identity must be unique. Applies only to a New Game.' 28 10 900 34)
    $factionIds = [string[]]@('GAIANS','HIVE','UNIV','MORGAN','SPARTANS','BELIEVE','PEACE',
        'CYBORG','PIRATES','DRONE','ANGELS','FUNGBOY','CARETAKE','USURPER')
    $factionTitles = [string[]]@(
        'Gaians - Deirdre','Human Hive - Yang','University - Zakharov',
        'Morgan Industries - Morgan','Spartan Federation - Santiago',
        'Lord''s Believers - Miriam','Peacekeepers - Lal',
        'Cybernetic Consciousness - Aki','Nautilus Pirates - Svensgaard',
        'Free Drones - Domai','Data Angels - Roze','Cult of Planet - Cha Dawn',
        'Manifold Caretakers - Hminee','Manifold Usurpers - Marr')
    $factionCombos = @()
    for ($slot = 1; $slot -le 7; ++$slot) {
        $key = 'faction_' + $slot
        $chosen = [Array]::IndexOf($factionIds, [string]$options[$key])
        if ($chosen -lt 0) { $chosen = $slot - 1 }
        $chooser = Add-Combo $factionTab ('Slot ' + $slot) (48 + ($slot-1)*81) $factionTitles $chosen
        $chooser.SetBounds(28, (78 + ($slot-1)*81), 620, 30)
        $factionCombos += $chooser
    }
    [void](Add-Label $factionTab 'The seven Alien Crossfire identities require SMAX enabled; the loader rejects unavailable expansion factions.' 28 645 900 58)

    [void](Add-Label $presentationTab 'Presentation mode' 30 30 860 26)
    $presentation = Add-Combo $presentationTab 'Select how much graphics to process' 74 @(
        'Normal - original presentation',
        'Fast - reduced animation and popup presentation',
        'Null Rendering - suppress routine map/tile drawing') (Get-Selected 'render_mode' 2)
    $telemetry = Add-Check $presentationTab 'Record campaign telemetry, including Council events' 181 ([string]$options['telemetry_enabled'] -eq '1')
    $notices = Add-Check $presentationTab 'Suppress audited noninteractive popups' 223 ([string]$options['suppress_notices'] -eq '1')
    $unlimitedExpansion = Add-Check $presentationTab 'Ignore the AI colony expansion cap while simulating (automation only)' 265 ([string]$options['unlimited_expansion'] -eq '1')
    $timing = Add-Check $presentationTab 'Log phase timings (UTC timestamps and elapsed milliseconds)' 306 ([string]$options['timing_enabled'] -eq '1')
    $archiveChoices = @(0,5,10,25,50,100)
    $archiveIndex = [Array]::IndexOf([int[]]$archiveChoices, [int]$options['archive_interval'])
    if ($archiveIndex -lt 0) { $archiveIndex = 3 }
    $archiveCadence = Add-Combo $presentationTab 'Archive a full-cycle SAV to the campaign every' 357 @(
        'Off', '5 turns', '10 turns', '25 turns', '50 turns', '100 turns') $archiveIndex
    [void](Add-Label $presentationTab 'Null Rendering only operates during eligible AI turns. AI visibility and fog-of-war state are unchanged.' 30 464 870 48)
    [void](Add-Label $presentationTab 'Archived checkpoints are separate from native autosaves; may consume substantial disk space on huge worlds.' 30 520 870 54)
    [void](Add-Label $presentationTab 'An unknown modal can still stop the engine. The external monitor warns but does not click it.' 30 580 870 54)

    [void](Add-Label $monitorTab 'Monitor runs in this batch console after launching the game.' 30 35 880 35)
    [void](Add-Label $monitorTab 'No-progress warning threshold (seconds)' 30 104 860 25)
    $warning = New-Object System.Windows.Forms.NumericUpDown
    $warning.Minimum = 30; $warning.Maximum = 60
    $warning.SetBounds(30,140,175,32)
    $threshold = 45
    if ([int]::TryParse([string]$options['monitor_seconds'],[ref]$threshold)) {
        $threshold=[Math]::Max(30,[Math]::Min(60,$threshold))
    }
    $warning.Value = [decimal]$threshold
    $monitorTab.Controls.Add($warning)
    [void](Add-Label $monitorTab 'Console commands:' 30 235 860 25)
    [void](Add-Label $monitorTab 'G = show game window     R = restore rendering     N = return to Null Rendering' 30 280 860 35)
    [void](Add-Label $monitorTab 'P = screenshot             S = stop at boundary       Space = pause at boundary' 30 325 860 35)
    [void](Add-Label $monitorTab 'Q = stop monitoring only (game remains running)' 30 370 860 35)
    [void](Add-Label $monitorTab 'A warning means no observed progress, not proof of a deadlock. No automatic termination.' 30 455 860 55)
    [void](Add-Label $monitorTab 'Restoring rendering cannot make an already-blocked game thread draw a fresh frame.' 30 520 860 55)

    # Campaign browsing and report actions are embedded in this launcher tab.
    # campaign_report.py is a headless data/rendering backend, not another GUI.
    [void](Add-Label $reportTab 'Campaign reports, session browser, saves and archive' 28 12 875 28)
    [void](Add-Label $reportTab 'Campaigns folder' 28 43 150 24)
    $rootBox = Add-Text $reportTab (Join-Path $gameRoot 'campaigns') 180 40 548
    $rootBox.ReadOnly = $true
    $rootBrowse = New-Object System.Windows.Forms.Button
    $rootBrowse.Text = 'Choose folder...'
    $rootBrowse.SetBounds(745,39,157,32)
    $reportTab.Controls.Add($rootBrowse)

    [void](Add-Label $reportTab 'Filter sessions' 28 85 120 24)
    $sessionFilter = Add-Text $reportTab '' 145 82 580
    $refreshCampaigns = New-Object System.Windows.Forms.Button
    $refreshCampaigns.Text = 'Refresh'
    $refreshCampaigns.SetBounds(745,79,157,34)
    $reportTab.Controls.Add($refreshCampaigns)

    $sessionList = New-Object System.Windows.Forms.ListView
    $sessionList.View = [System.Windows.Forms.View]::Details
    $sessionList.FullRowSelect = $true
    $sessionList.MultiSelect = $false
    $sessionList.HideSelection = $false
    $sessionList.GridLines = $true
    $sessionList.ShowItemToolTips = $true
    $sessionList.SetBounds(28,129,874,266)
    # Keep columns within viewport; each session is one ListViewItem/row.
    [void]$sessionList.Columns.Add('Campaign',190)
    [void]$sessionList.Columns.Add('Session',80)
    [void]$sessionList.Columns.Add('Faction / observer',140)
    [void]$sessionList.Columns.Add('Years',108)
    [void]$sessionList.Columns.Add('Turns',55)
    [void]$sessionList.Columns.Add('Bases',54)
    [void]$sessionList.Columns.Add('Status',85)
    [void]$sessionList.Columns.Add('Updated',130)
    # A generic List (not nested PowerShell arrays) prevents the JSON results
    # being grouped into one GUI item by implicit pipeline enumeration.
    $sessionList.Tag = (New-Object 'System.Collections.Generic.List[object]')
    $reportTab.Controls.Add($sessionList)

    [void](Add-Label $reportTab 'Selected recording' 28 405 870 24)
    $sessionDetails = New-Object System.Windows.Forms.TextBox
    $sessionDetails.Multiline = $true
    $sessionDetails.ReadOnly = $true
    $sessionDetails.ScrollBars = 'Vertical'
    $sessionDetails.SetBounds(28,431,874,102)
    $reportTab.Controls.Add($sessionDetails)

    $generateReport = New-Object System.Windows.Forms.Button
    $generateReport.Text = 'Generate + open report'
    $generateReport.SetBounds(28,548,208,42)
    $reportTab.Controls.Add($generateReport)
    $openExisting = New-Object System.Windows.Forms.Button
    $openExisting.Text = 'Open existing report'
    $openExisting.SetBounds(248,548,199,42)
    $reportTab.Controls.Add($openExisting)
    $openCampaign = New-Object System.Windows.Forms.Button
    $openCampaign.Text = 'Open selected session folder'
    $openCampaign.SetBounds(459,548,202,42)
    $reportTab.Controls.Add($openCampaign)
    $zipCampaign = New-Object System.Windows.Forms.Button
    $zipCampaign.Text = 'Export campaign ZIP'
    $zipCampaign.SetBounds(673,548,229,42)
    $reportTab.Controls.Add($zipCampaign)

    $openAutomationLogs = New-Object System.Windows.Forms.Button
    $openAutomationLogs.Text = 'Open launcher logs'
    $openAutomationLogs.SetBounds(28,608,206,36)
    $reportTab.Controls.Add($openAutomationLogs)
    $reportStatus = Add-Label $reportTab 'Select a recorded session. Reports use Python 3; launching the game does not.' 249 605 650 58
    [void](Add-Label $reportTab 'Archives include SAV checkpoints, raw JSONL, metadata and generated reports. Prefer cleanly closed sessions for exports.' 28 675 875 38)

    $archiveRoot = Join-Path $gameRoot 'automation\archives'
    $getSelectedRow = {
        if ($sessionList.SelectedItems.Count -eq 0) { return $null }
        return $sessionList.SelectedItems[0].Tag
    }
    $updateReportDetails = {
        $row = & $getSelectedRow
        $hasRow = $null -ne $row
        $generateReport.Enabled = $hasRow -and -not [bool]$row.error
        $openExisting.Enabled = $hasRow
        $openCampaign.Enabled = $hasRow
        $zipCampaign.Enabled = $hasRow
        if (-not $hasRow) {
            $sessionDetails.Text = 'No session selected. Record a game with telemetry enabled, or choose a different campaigns folder.'
        } elseif ($row.error) {
            $sessionDetails.Text = "Campaign: $($row.campaign_name) / $($row.session_id)" +
                [Environment]::NewLine + "Unreadable recording: $($row.error)"
        } else {
            $sessionDetails.Text = "Campaign: $($row.campaign_name)  /  Session: $($row.session_id)" +
                [Environment]::NewLine + "Folder: $($row.campaign_dir)" +
                [Environment]::NewLine + "Started: $($row.started)  |  State: $($row.status)  |  AI runtime: $($row.runtime)" +
                [Environment]::NewLine + "Source save: $($row.source_save)" +
                [Environment]::NewLine + "Living factions: $($row.factions)"
        }
    }
    $displayCampaignRows = {
        $priorPath = ''
        $prior = & $getSelectedRow
        if ($prior) { $priorPath = [string]$prior.campaign_dir + '\' + [string]$prior.session_id }
        $term = $sessionFilter.Text.Trim().ToLowerInvariant()
        $sessionList.BeginUpdate()
        try {
            $sessionList.Items.Clear()
            foreach ($row in $sessionList.Tag) {
                if ($null -eq $row) { continue }
                $haystack = @($row.folder, $row.campaign_name, $row.player,
                    $row.years, $row.status, $row.started, $row.updated,
                    $row.source_save, $row.factions, $row.session_id) -join ' '
                if ($term -and -not $haystack.ToLowerInvariant().Contains($term)) { continue }
                $item = New-Object System.Windows.Forms.ListViewItem([string]$row.campaign_name)
                foreach ($value in @($row.session_id, $row.player, $row.years,
                    $row.turns, $row.bases, $row.status, $row.updated)) {
                    [void]$item.SubItems.Add([string]$value)
                }
                $item.Tag = $row
                $item.ToolTipText = ([string]$row.campaign_name + ' / ' +
                    [string]$row.session_id + ' / ' + [string]$row.campaign_dir)
                [void]$sessionList.Items.Add($item)
                if ($priorPath -eq ([string]$row.campaign_dir + '\' + [string]$row.session_id)) {
                    $item.Selected = $true
                }
            }
            if ($sessionList.SelectedItems.Count -eq 0 -and $sessionList.Items.Count -gt 0) {
                $sessionList.Items[0].Selected = $true
            }
        } finally {
            $sessionList.EndUpdate()
        }
        & $updateReportDetails
    }
    $refreshAction = {
        try {
            $root = [System.IO.Path]::GetFullPath($rootBox.Text)
            $json = Invoke-CampaignReporter -arguments @('--scan-json', $root)
            $rows = Convert-CampaignSessionRows $json
            $sessionList.Tag.Clear()
            foreach ($row in $rows) { [void]$sessionList.Tag.Add($row) }
            & $displayCampaignRows
            $reportStatus.Text = "$(@($sessionList.Tag).Count) session(s) found. Select, filter, generate or export."
        } catch {
            # Pure PowerShell fallback: keep folder/ZIP actions functional
            # without Python or when a reporter dependency cannot be loaded.
            # Metadata/report generation remains explicitly unavailable.
            $scanFailure = $_.Exception.Message
            $fallbackRows = New-Object 'System.Collections.Generic.List[object]'
            try {
                if ([System.IO.Directory]::Exists($rootBox.Text)) {
                    foreach ($folder in @(Get-ChildItem -LiteralPath $rootBox.Text -Directory)) {
                        $sessionsPath = Join-Path $folder.FullName 'sessions'
                        if (-not [System.IO.Directory]::Exists($sessionsPath)) { continue }
                        foreach ($session in @(Get-ChildItem -LiteralPath $sessionsPath -Directory)) {
                            $fallbackRows.Add([pscustomobject]@{
                                campaign_dir = $folder.FullName
                                session_id = $session.Name
                                folder = $folder.Name
                                campaign_name = $folder.Name
                                player = 'Python unavailable'
                                years = '—'
                                turns = '—'
                                bases = '—'
                                units = '—'
                                status = 'Unscanned'
                                started = 'Unknown'
                                runtime = 'Not recorded'
                                updated = $session.LastWriteTime.ToString('dd MMM yyyy HH:mm')
                                source_save = 'Not scanned'
                                factions = ''
                                error = 'Report scan/generation needs Python 3'
                            })
                        }
                    }
                }
            } catch {
                $scanFailure += '; fallback directory scan also failed: ' + $_.Exception.Message
            }
            $sessionList.Tag.Clear()
            foreach ($row in $fallbackRows) { [void]$sessionList.Tag.Add($row) }
            & $displayCampaignRows
            $reportStatus.Text = 'Report scan unavailable: ' + $scanFailure +
                '. Folder and ZIP actions remain available.'
        }
    }
    $refreshCampaigns.Add_Click({ & $refreshAction })
    $sessionFilter.Add_TextChanged({ & $displayCampaignRows })
    $sessionList.Add_SelectedIndexChanged({ & $updateReportDetails })
    $rootBrowse.Add_Click({
        $picker = New-Object System.Windows.Forms.FolderBrowserDialog
        try {
            $picker.Description = 'Select the folder containing your campaign directories'
            $picker.SelectedPath = $rootBox.Text
            if ($picker.ShowDialog($form) -eq [System.Windows.Forms.DialogResult]::OK) {
                $rootBox.Text = [System.IO.Path]::GetFullPath($picker.SelectedPath)
                & $refreshAction
            }
        } finally { $picker.Dispose() }
    })
    $generateReportAction = {
        $row = & $getSelectedRow
        if (-not $row -or $row.error) { return }
        try {
            $form.UseWaitCursor = $true
            $reportStatus.Text = 'Generating offline report for ' + $row.session_id + '...'
            $reportStatus.Refresh()
            [void](Invoke-CampaignReporter -arguments @([string]$row.campaign_dir,
                '--session', [string]$row.session_id))
            $reportPath = Join-Path (Join-Path (Join-Path ([string]$row.campaign_dir) 'reports') ([string]$row.session_id)) 'index.html'
            if (-not [System.IO.File]::Exists($reportPath)) {
                throw 'Reporter completed without creating index.html.'
            }
            $reportStatus.Text = 'Report created: ' + $reportPath
            Invoke-Item -LiteralPath $reportPath
        } catch {
            $reportStatus.Text = 'Report generation failed.'
            [void][System.Windows.Forms.MessageBox]::Show(
                $_.Exception.Message, 'Campaign report',
                [System.Windows.Forms.MessageBoxButtons]::OK,
                [System.Windows.Forms.MessageBoxIcon]::Error)
        } finally { $form.UseWaitCursor = $false }
    }
    $generateReport.Add_Click({ & $generateReportAction })
    $sessionList.Add_DoubleClick({ & $generateReportAction })
    $openExisting.Add_Click({
        $row = & $getSelectedRow
        if (-not $row) { return }
        $reportPath = Join-Path (Join-Path (Join-Path ([string]$row.campaign_dir) 'reports') ([string]$row.session_id)) 'index.html'
        if ([System.IO.File]::Exists($reportPath)) {
            Invoke-Item -LiteralPath $reportPath
        } else {
            [void][System.Windows.Forms.MessageBox]::Show(
                'No report has been generated for this session yet. Choose Generate + open report.',
                'Campaign report',
                [System.Windows.Forms.MessageBoxButtons]::OK,
                [System.Windows.Forms.MessageBoxIcon]::Information)
        }
    })
    $openCampaign.Add_Click({
        $row = & $getSelectedRow
        if (-not $row) { return }
        $sessionFolder = Join-Path (Join-Path ([string]$row.campaign_dir) 'sessions') ([string]$row.session_id)
        if ([System.IO.Directory]::Exists($sessionFolder)) {
            Invoke-Item -LiteralPath $sessionFolder
        } else {
            [void][System.Windows.Forms.MessageBox]::Show(
                'Selected session folder no longer exists. Refresh the list.',
                'Campaign session', [System.Windows.Forms.MessageBoxButtons]::OK,
                [System.Windows.Forms.MessageBoxIcon]::Warning)
        }
    })
    $zipCampaign.Add_Click({
        try {
            $row = & $getSelectedRow
            if (-not $row) { throw 'Select a recorded session before exporting its campaign.' }
            $campaignFolder = [System.IO.Path]::GetFullPath([string]$row.campaign_dir)
            if (-not [System.IO.Directory]::Exists($campaignFolder)) {
                throw 'Selected campaign folder no longer exists. Refresh the list.'
            }
            [void][System.IO.Directory]::CreateDirectory($archiveRoot)
            $suggested = [string]$row.folder + '-' +
                (Get-Date -Format 'yyyyMMdd-HHmmss') + '.zip'
            $picker = New-Object System.Windows.Forms.SaveFileDialog
            try {
                $picker.Filter = 'ZIP archive (*.zip)|*.zip'
                $picker.FileName = $suggested
                $picker.InitialDirectory = $archiveRoot
                $picker.OverwritePrompt = $true
                if ($picker.ShowDialog($form) -ne [System.Windows.Forms.DialogResult]::OK) { return }
                $destination = [System.IO.Path]::GetFullPath($picker.FileName)
            } finally {
                $picker.Dispose()
            }
            if ($destination.StartsWith(
                $campaignFolder.TrimEnd('\') + '\',
                [StringComparison]::OrdinalIgnoreCase)) {
                throw 'ZIP destination must be outside the campaign being archived.'
            }
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            $temporary = $destination + '.pending-' + [Guid]::NewGuid().ToString('N')
            try {
                [System.IO.Compression.ZipFile]::CreateFromDirectory(
                    $campaignFolder, $temporary,
                    [System.IO.Compression.CompressionLevel]::Optimal, $true)
                if ([System.IO.File]::Exists($destination)) {
                    [System.IO.File]::Replace($temporary, $destination, $null)
                } else {
                    [System.IO.File]::Move($temporary, $destination)
                }
            } finally {
                if ([System.IO.File]::Exists($temporary)) {
                    [System.IO.File]::Delete($temporary)
                }
            }
            $reportStatus.Text = 'Campaign ZIP created: ' + $destination
        } catch {
            [void][System.Windows.Forms.MessageBox]::Show(
                $_.Exception.Message, 'Campaign archive error',
                [System.Windows.Forms.MessageBoxButtons]::OK,
                [System.Windows.Forms.MessageBoxIcon]::Error)
        }
    })
    $openAutomationLogs.Add_Click({
        $folder = Join-Path $gameRoot 'automation\logs'
        if ([System.IO.Directory]::Exists($folder)) { Invoke-Item -LiteralPath $folder }
    })
    if (-not $GuiSelfTest) { & $refreshAction }
    else { & $updateReportDetails }

    $start = New-Object System.Windows.Forms.Button
    $start.Text = 'Launch Simulation'
    $start.SetBounds(783,784,215,44)
    $form.Controls.Add($start)
    $cancel = New-Object System.Windows.Forms.Button
    $cancel.Text = 'Cancel'
    $cancel.SetBounds(649,784,125,44)
    $form.Controls.Add($cancel)
    $form.CancelButton = $cancel
    $cancel.Add_Click({
        $form.DialogResult=[System.Windows.Forms.DialogResult]::Cancel
        $form.Close()
    })
    if ([string]$options['mode'] -eq 'load') { $loadGame.Checked=$true }
    else { $newGame.Checked=$true }
    $updateMode = {
        $source.Enabled=$loadGame.Checked
        $browse.Enabled=$loadGame.Checked
        $worldTab.Enabled=$newGame.Checked
        $rulesTab.Enabled=$newGame.Checked
        $factionTab.Enabled=$newGame.Checked
    }
    $newGame.Add_CheckedChanged($updateMode)
    $loadGame.Add_CheckedChanged($updateMode)
    & $updateMode
    $browse.Add_Click({
        $picker = New-Object System.Windows.Forms.OpenFileDialog
        $picker.Filter = 'SMAC/X saves (*.sav)|*.sav'
        $picker.CheckFileExists=$true
        $picker.InitialDirectory = Join-Path $gameRoot 'saves'
        if (-not [System.IO.Directory]::Exists($picker.InitialDirectory)) {
            $picker.InitialDirectory=$gameRoot
        }
        if ($picker.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
            $source.Text=$picker.FileName
        }
        $picker.Dispose()
    })
    $script:selectedProfile=$null
    $script:warningSeconds=45
    $start.Add_Click({
        try {
            $name=$campaign.Text.Trim()
            if ($name.Length -lt 1 -or $name.Length -gt 88 -or
                $name -match '[\r\n;#]') {
                throw 'Campaign name must be 1-88 characters, without newlines, ; or #.'
            }
            $mode = if ($loadGame.Checked) {'load'} else {'new'}
            $save=''
            if ($mode -eq 'load') {
                $save=[System.IO.Path]::GetFullPath($source.Text.Trim())
                if (-not $save.EndsWith('.sav',[StringComparison]::OrdinalIgnoreCase) -or
                    -not [System.IO.File]::Exists($save) -or
                    $save -match '[\r\n]') { throw 'Select an existing SAV.' }
            }
            if ($archiveCadence.SelectedIndex -gt 0 -and -not $telemetry.Checked) {
                throw 'Checkpoint archiving requires campaign telemetry to be enabled.'
            }
            $chosenFactions = @()
            foreach ($selector in $factionCombos) {
                $id = [string]$factionIds[$selector.SelectedIndex]
                if ($chosenFactions -contains $id) {
                    throw ('Faction ' + $id + ' is assigned to multiple slots. Select seven different identities.')
                }
                $chosenFactions += $id
            }
            # Explicit normal custom-game bits; the native checkbox for
            # Unity Survey has opposite polarity to RULES_NO_UNITY_SURVEY.
            $rulesMask = 0
            foreach ($entry in $ruleControls) {
                $selected = [bool]$entry.Control.Checked
                if ($entry.Inverted) { $selected = -not $selected }
                if ($selected) { $rulesMask = $rulesMask -bor [int]$entry.Bit }
            }
            $randomLeaderBits = 0
            if ($randomPersonalities.Checked) { $randomLeaderBits = $randomLeaderBits -bor 1 }
            if ($randomAgendas.Checked) { $randomLeaderBits = $randomLeaderBits -bor 2 }
            if ($mode -eq 'new' -and $worldSource.SelectedIndex -gt 0) {
                $mapName = if ($worldSource.SelectedIndex -eq 1) {
                    'xplanet.mp'
                } else { 'xplanetx.mp' }
                $mapPath = Join-Path $gameRoot ('maps\' + $mapName)
                if (-not [System.IO.File]::Exists($mapPath)) {
                    throw ('Selected Planet map is missing: ' + $mapPath)
                }
            }
            if ($mode -eq 'new' -and $worldSource.SelectedIndex -eq 0 -and
                $worldSize.SelectedIndex -eq 6 -and
                ($customX.Value -gt 256 -or $customY.Value -gt 256)) {
                $choice = [System.Windows.Forms.MessageBox]::Show(
                    $form,
                    'One or both custom map dimensions exceed 256. Very large maps ' +
                    'may have high memory and save/reload costs. Launch anyway?',
                    'Very large custom map',
                    [System.Windows.Forms.MessageBoxButtons]::YesNo,
                    [System.Windows.Forms.MessageBoxIcon]::Warning)
                if ($choice -ne [System.Windows.Forms.DialogResult]::Yes) { return }
            }
            $render=[int]$presentation.SelectedIndex
            $updated = @{
                mode=$mode
                source_save=$save
                campaign_name=$name
                turns=[string][int]$turns.Value
                telemetry_enabled=if ($telemetry.Checked) {'1'} else {'0'}
                suppress_notices=if ($notices.Checked) {'1'} else {'0'}
                unlimited_expansion=if ($unlimitedExpansion.Checked) {'1'} else {'0'}
                timing_enabled=if ($timing.Checked) {'1'} else {'0'}
                archive_interval=[string]$archiveChoices[$archiveCadence.SelectedIndex]
                fast_combat=if ($render -ge 1) {'1'} else {'0'}
                render_mode=[string]$render
                world_source=[string][int]$worldSource.SelectedIndex
                world_size=[string][int]$worldSize.SelectedIndex
                custom_map_x=[string][int]$customX.Value
                custom_map_y=[string][int]$customY.Value
                rules_mode=if ($customRules.Checked) {'1'} else {'0'}
                rules_mask=[string]$rulesMask
                rules_random=[string]$randomLeaderBits
                world_ocean=[string][int]$worldOcean.SelectedIndex
                world_erosion=[string][int]$worldErosion.SelectedIndex
                world_cloud=[string][int]$worldCloud.SelectedIndex
                world_life=[string][int]$worldLife.SelectedIndex
                difficulty=[string][int]$difficulty.SelectedIndex
                observer=[string]($observer.SelectedIndex + 1)
                monitor_seconds=[string][int]$warning.Value
            }
            for ($slot = 1; $slot -le 7; ++$slot) {
                $updated['faction_' + $slot] = [string]$chosenFactions[$slot-1]
            }
            Save-Options $prefsPath $updated $false
            $script:selectedProfile=New-RunProfile $gameRoot $updated
            $script:warningSeconds=[int]$warning.Value
            $form.DialogResult=[System.Windows.Forms.DialogResult]::OK
            $form.Close()
        } catch {
            [void][System.Windows.Forms.MessageBox]::Show(
                $_.Exception.Message,'Automation launcher',
                [System.Windows.Forms.MessageBoxButtons]::OK,
                [System.Windows.Forms.MessageBoxIcon]::Warning)
        }
    })
    if ($GuiSelfTest) {
        if ($tabs.TabPages.Count -ne 7 -or $factionCombos.Count -ne 7 -or
            $generateReport.Text -ne 'Generate + open report' -or
            $openExisting.Text -ne 'Open existing report' -or
            $sessionList.Columns.Count -ne 8 -or
            $rootBrowse.Text -ne 'Choose folder...' -or
            $presentation.Items.Count -ne 3 -or
            $worldSize.Items.Count -ne 7 -or $worldSource.Items.Count -ne 3 -or
            $customX.Minimum -ne 16 -or $customX.Maximum -ne 512 -or
            $customY.Minimum -ne 16 -or $customY.Maximum -ne 512 -or
            $ruleControls.Count -ne 16 -or
            $rulesTab.Text -ne 'Game rules & victory' -or
            $factionCombos.Count -ne 7 -or
            $archiveCadence.Items.Count -ne 6 -or $turns.Maximum -ne 100000) {
            throw 'Windows WinForms launcher controls failed the GUI smoke test.'
        }
        # Exercise the real WinForms row binding, not just JSON parsing.
        # Two sessions in one campaign and a second campaign must appear
        # as three independent clickable ListViewItem objects.
        function New-GuiCampaignTestRow([string]$name, [string]$folder, [string]$session) {
            return [pscustomobject]@{
                campaign_dir = ('C:\ci\campaigns\' + $folder)
                folder = $folder
                campaign_name = $name
                session_id = $session
                player = 'Gaia'
                years = '2101-2111'
                turns = '11'
                bases = '25'
                status = 'Closed'
                started = '9 Nov 2026'
                updated = '9 Nov 2026'
                runtime = '1m'
                source_save = 'synthetic'
                factions = 'Gaia, Hive'
                error = $null
            }
        }
        $guiFixture = @(
            (New-GuiCampaignTestRow 'Campaign One' 'campaign-one' 'session-001'),
            (New-GuiCampaignTestRow 'Campaign One' 'campaign-one' 'session-002'),
            (New-GuiCampaignTestRow 'Campaign Two' 'campaign-two' 'session-001')
        )
        $guiJson = ConvertTo-Json -InputObject $guiFixture -Depth 4 -Compress
        $guiRows = Convert-CampaignSessionRows $guiJson
        $sessionList.Tag.Clear()
        foreach ($row in $guiRows) { [void]$sessionList.Tag.Add($row) }
        & $displayCampaignRows
        if ($sessionList.Items.Count -ne 3) {
            throw ('Campaign picker collapsed independent sessions into ' +
                $sessionList.Items.Count + ' row(s).')
        }
        if ([string]$sessionList.Items[0].Tag.session_id -ne 'session-001' -or
            [string]$sessionList.Items[1].Tag.session_id -ne 'session-002' -or
            [string]$sessionList.Items[2].Tag.campaign_name -ne 'Campaign Two') {
            throw 'Campaign picker lost per-session ListViewItem.Tag selection identity.'
        }
        # Filtering must preserve game/session identity, rather than presenting
        # all matching campaigns as a joined single-row string.
        $sessionFilter.Text = 'Campaign Two'
        & $displayCampaignRows
        if ($sessionList.Items.Count -ne 1 -or
            [string]$sessionList.Items[0].Tag.campaign_name -ne 'Campaign Two') {
            throw 'Campaign picker multi-row filtering failed.'
        }
        $sessionFilter.Text = ''
        & $displayCampaignRows
        if ($sessionList.Items.Count -ne 3) {
            throw 'Clearing the campaign filter did not restore all sessions.'
        }
        $sessionList.Tag.Clear()
        $sessionList.Items.Clear()
        $sessionFilter.Text = ''
        Write-Host 'AUTO-LAUNCH multi-campaign WinForms row binding passed'
        $worldSource.SelectedIndex = 1
        if ($worldSize.Enabled -or $worldOcean.Enabled -or $customX.Enabled) {
            throw 'Fixed-Planet source must disable random size/climate/custom fields.'
        }
        $worldSource.SelectedIndex = 0
        $worldSize.SelectedIndex = 6
        if (-not $customX.Enabled -or -not $customY.Enabled -or
            -not $worldOcean.Enabled) {
            throw 'Generated custom map must enable X/Y and climate.'
        }
        $customRules.Checked = $true
        if (-not $ruleControls[0].Control.Enabled -or
            -not $randomPersonalities.Enabled) {
            throw 'Enabling explicit game rules must enable all rule selectors.'
        }
        $customRules.Checked = $false
        if ($ruleControls[0].Control.Enabled) {
            throw 'Disabled custom rules must defer to installed game settings.'
        }
        $form.Dispose()
        Write-Host 'AUTO-LAUNCH Windows WinForms GUI construction passed'
        exit 0
    }
    Write-Host 'Pre-game setup GUI ready.'
    $result=$form.ShowDialog()
    $form.Dispose()
    if ($result -ne [System.Windows.Forms.DialogResult]::OK -or
        -not $script:selectedProfile) {
        Write-Host 'User cancelled pre-game setup; no simulation launched.'
        exit 0
    }
    Write-Host ('Selected profile: ' + $script:selectedProfile)
    Write-Host 'Starting existing thinker.exe and its terranx.exe child.'

    $startInfo=New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName=Join-Path $gameRoot 'thinker.exe'
    $startInfo.WorkingDirectory=$gameRoot
    $startInfo.UseShellExecute=$false
    $startInfo.EnvironmentVariables['THINKERNECA_AUTOSIM_PROFILE'] =
        [string]$script:selectedProfile
    $launcher=[System.Diagnostics.Process]::Start($startInfo)
    if (-not $launcher) {throw 'Could not start thinker.exe'}
    $launcherPid = [int]$launcher.Id
    Write-Host ('thinker.exe launcher PID: ' + $launcherPid)
    $launcher.Dispose()
    Run-Monitor ([string]$script:selectedProfile) $script:warningSeconds $launcherPid
    Write-Host 'Monitor finished. The batch console remains open for diagnostics.'
} catch {
    Write-Host ('AUTOMATION LAUNCH ERROR: ' + $_.Exception.ToString())
    if ($_.ScriptStackTrace) { Write-Host ('PowerShell stack: ' + $_.ScriptStackTrace) }
    try {
        [void][System.Windows.Forms.MessageBox]::Show(
            $_.Exception.Message,'thinkerNeca Automation Launcher',
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Error)
    } catch {
        Write-Host ('Error dialog unavailable: ' + $_.Exception.Message)
    }
    exit 1
} finally {
    if ($script:launcherTranscriptStarted) {
        try { Stop-Transcript | Out-Null } catch {}
    }
}
