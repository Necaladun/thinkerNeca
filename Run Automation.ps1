param([switch]$SelfTest, [switch]$GuiSelfTest)

# Stage A launch GUI. Nothing in thinker.ini, thinker_user.ini or the game's
# Alpha Centauri.ini is written. The same thinker.exe is always launched.
$ErrorActionPreference = 'Stop'
$gameRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $MyInvocation.MyCommand.Path))
$prefsPath = Join-Path $gameRoot 'automation.ini'

# Independent launcher log is created BEFORE starting the game.
# It remains available even if terranx crashes before telemetry starts.
$script:launcherLogPath = $null
$script:launcherTranscriptStarted = $false
if (-not $SelfTest) {
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
        world_size = '3'
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
                      'render_mode', 'world_size', 'world_ocean', 'world_erosion',
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
            -not $lines.Contains('world_size=3') -or
            -not $lines.Contains('monitor_seconds=45')) {
            throw 'run-profile snapshot mismatch'
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

    [void](Add-Label $worldTab 'These options apply only to New Game.' 28 21 870 30)
    $difficulty = Add-Combo $worldTab 'Difficulty' 75 @('Citizen','Specialist','Talent','Librarian','Thinker','Transcend') (Get-Selected 'difficulty' 5)
    $worldSize = Add-Combo $worldTab 'World size' 159 @('Tiny','Small','Standard','Large','Huge') (Get-Selected 'world_size' 4)
    $worldOcean = Add-Combo $worldTab 'Ocean coverage' 243 @('Low','Average','High') (Get-Selected 'world_ocean' 2)
    $worldErosion = Add-Combo $worldTab 'Erosion' 327 @('Low','Average','High') (Get-Selected 'world_erosion' 2)
    $worldCloud = Add-Combo $worldTab 'Cloud cover' 411 @('Low','Average','High') (Get-Selected 'world_cloud' 2)
    $worldLife = Add-Combo $worldTab 'Native life' 495 @('Low','Average','High') (Get-Selected 'world_life' 2)
    $observer = Add-Combo $worldTab 'Initial observer (not a human faction)' 579 @('Faction slot 1','Faction slot 2','Faction slot 3','Faction slot 4','Faction slot 5','Faction slot 6','Faction slot 7') ([Math]::Max(0,(Get-Selected 'observer' 7)-1))

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

    [void](Add-Label $reportTab 'Reports, campaign archives and diagnostic files from one launcher.' 28 20 910 34)
    [void](Add-Label $reportTab 'Selected recorded campaign' 28 77 850 25)
    $campaignList = New-Object System.Windows.Forms.ComboBox
    $campaignList.DropDownStyle = 'DropDownList'
    $campaignList.SetBounds(28,107,700,32)
    $reportTab.Controls.Add($campaignList)
    $refreshCampaigns = New-Object System.Windows.Forms.Button
    $refreshCampaigns.Text = 'Refresh'
    $refreshCampaigns.SetBounds(749,107,150,34)
    $reportTab.Controls.Add($refreshCampaigns)
    $campaignRoot = Join-Path $gameRoot 'campaigns'
    $archiveRoot = Join-Path $gameRoot 'automation\archives'
    $refreshAction = {
        $current = [string]$campaignList.SelectedItem
        $campaignList.Items.Clear()
        if ([System.IO.Directory]::Exists($campaignRoot)) {
            Get-ChildItem -LiteralPath $campaignRoot -Directory |
                Sort-Object LastWriteTime -Descending | ForEach-Object {
                    [void]$campaignList.Items.Add($_.Name)
                }
        }
        if ($campaignList.Items.Count -gt 0) {
            $index = $campaignList.Items.IndexOf($current)
            $campaignList.SelectedIndex = if ($index -ge 0) {$index} else {0}
        }
    }
    $refreshCampaigns.Add_Click($refreshAction)
    & $refreshAction
    $reportAnalyzer = New-Object System.Windows.Forms.Button
    $reportAnalyzer.Text = 'Open Python Campaign Analyzer'
    $reportAnalyzer.SetBounds(28,180,300,49)
    $reportTab.Controls.Add($reportAnalyzer)
    $reportAnalyzer.Add_Click({
        try {
            $scriptPath = Join-Path $gameRoot 'tools\Campaign Analyzer.pyw'
            if (-not [System.IO.File]::Exists($scriptPath)) {
                throw 'Campaign Analyzer.pyw not installed. Install the tools/ folder from the automation build.'
            }
            $python = $null
            $extra = @()
            foreach ($candidate in @('pythonw.exe','pyw.exe','python.exe','py.exe')) {
                $cmd = Get-Command $candidate -ErrorAction SilentlyContinue |
                    Select-Object -First 1
                if ($cmd) {
                    $python = $cmd.Source
                    if ($candidate -like 'py*' -and $candidate -ne 'pythonw.exe' -and $candidate -ne 'python.exe') {
                        $extra = @('-3')
                    }
                    break
                }
            }
            if (-not $python) { throw 'Install Python 3 (including Tkinter) to open Campaign Analyzer.' }
            $argsList = @($extra) + @('"' + $scriptPath + '"', '"' + $campaignRoot + '"')
            [void](Start-Process -FilePath $python -WorkingDirectory (Join-Path $gameRoot 'tools') -ArgumentList $argsList -PassThru)
        } catch {
            [void][System.Windows.Forms.MessageBox]::Show(
                $_.Exception.Message, 'Campaign Analyzer',
                [System.Windows.Forms.MessageBoxButtons]::OK,
                [System.Windows.Forms.MessageBoxIcon]::Warning)
        }
    })
    $openCampaign = New-Object System.Windows.Forms.Button
    $openCampaign.Text = 'Open campaign folder'
    $openCampaign.SetBounds(343,180,245,49)
    $reportTab.Controls.Add($openCampaign)
    $openCampaign.Add_Click({
        if ($campaignList.SelectedIndex -ge 0) {
            $directory = Join-Path $campaignRoot ([string]$campaignList.SelectedItem)
            if ([System.IO.Directory]::Exists($directory)) { Invoke-Item -LiteralPath $directory }
        }
    })
    $zipCampaign = New-Object System.Windows.Forms.Button
    $zipCampaign.Text = 'Export selected campaign ZIP'
    $zipCampaign.SetBounds(28,271,350,49)
    $reportTab.Controls.Add($zipCampaign)
    $zipCampaign.Add_Click({
        try {
            if ($campaignList.SelectedIndex -lt 0) {
                throw 'Select an existing recorded campaign first.'
            }
            $campaignFolder = Join-Path $campaignRoot ([string]$campaignList.SelectedItem)
            if (-not [System.IO.Directory]::Exists($campaignFolder)) {
                throw 'Selected campaign folder no longer exists. Refresh the list.'
            }
            [void][System.IO.Directory]::CreateDirectory($archiveRoot)
            $suggested = [string]$campaignList.SelectedItem + '-' +
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
                [System.IO.Path]::GetFullPath($campaignFolder).TrimEnd('\') + '\',
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
                    # Atomic replacement keeps the previous archive intact if
                    # final commit fails. Temporary and target share a volume.
                    [System.IO.File]::Replace($temporary, $destination, $null)
                } else {
                    [System.IO.File]::Move($temporary, $destination)
                }
            } finally {
                if ([System.IO.File]::Exists($temporary)) {
                    [System.IO.File]::Delete($temporary)
                }
            }
            [void][System.Windows.Forms.MessageBox]::Show(
                ('Campaign ZIP created: ' + $destination),
                'Campaign archive',
                [System.Windows.Forms.MessageBoxButtons]::OK,
                [System.Windows.Forms.MessageBoxIcon]::Information)
        } catch {
            [void][System.Windows.Forms.MessageBox]::Show(
                $_.Exception.Message, 'Campaign archive error',
                [System.Windows.Forms.MessageBoxButtons]::OK,
                [System.Windows.Forms.MessageBoxIcon]::Error)
        }
    })
    [void](Add-Label $reportTab 'A campaign ZIP includes its completed-cycle SAV checkpoints, reports, manifest, result, phase timings and all JSONL streams.' 28 355 890 62)
    [void](Add-Label $reportTab 'If a session is still running, logs may be changing during ZIP creation. Prefer archiving after a clean pause or exit.' 28 430 890 58)
    $openAutomationLogs = New-Object System.Windows.Forms.Button
    $openAutomationLogs.Text = 'Open launcher logs'
    $openAutomationLogs.SetBounds(28,535,290,45)
    $reportTab.Controls.Add($openAutomationLogs)
    $openAutomationLogs.Add_Click({
        $folder = Join-Path $gameRoot 'automation\logs'
        if ([System.IO.Directory]::Exists($folder)) { Invoke-Item -LiteralPath $folder }
    })

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
                world_size=[string][int]$worldSize.SelectedIndex
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
        if ($tabs.TabPages.Count -ne 6 -or $factionCombos.Count -ne 7 -or
            $reportAnalyzer.Text -ne 'Open Python Campaign Analyzer' -or
            $presentation.Items.Count -ne 3 -or
            $worldSize.Items.Count -ne 5 -or $factionCombos.Count -ne 7 -or
            $archiveCadence.Items.Count -ne 6 -or $turns.Maximum -ne 100000) {
            throw 'Windows WinForms launcher controls failed the GUI smoke test.'
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
