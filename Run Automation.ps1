param([switch]$SelfTest)

# Stage A launch GUI. Nothing in thinker.ini, thinker_user.ini or the game's
# Alpha Centauri.ini is written. The same thinker.exe is always launched.
$ErrorActionPreference = 'Stop'
$gameRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $MyInvocation.MyCommand.Path))
$prefsPath = Join-Path $gameRoot 'automation.ini'

function New-Defaults {
    return @{
        mode = 'new'
        source_save = ''
        campaign_name = ('AI Simulation ' + (Get-Date -Format 'yyyy-MM-dd HHmm'))
        turns = '100'
        telemetry_enabled = '1'
        suppress_notices = '1'
        fast_combat = '1'
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
                      'turns', 'telemetry_enabled', 'suppress_notices', 'fast_combat')) {
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
            -not $lines.Contains('turns=100')) {
            throw 'run-profile snapshot mismatch'
        }
        Write-Output 'AUTO-LAUNCH Stage A PowerShell profile smoke passed'
    } finally {
        [System.IO.Directory]::Delete($root, $true)
    }
    exit 0
}

try {
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    [System.Windows.Forms.Application]::EnableVisualStyles()
    foreach ($filename in @('thinker.exe', 'thinker.dll', 'terranx.exe')) {
        if (-not [System.IO.File]::Exists((Join-Path $gameRoot $filename))) {
            throw ('Missing ' + $filename + ' beside Run Automation.bat')
        }
    }
    $options = Read-Options $prefsPath

    $form = New-Object System.Windows.Forms.Form
    $form.Text = 'thinkerNeca - Automation Launcher'
    $form.Size = New-Object System.Drawing.Size(690, 595)
    $form.MinimumSize = $form.MaximumSize = $form.Size
    $form.StartPosition = 'CenterScreen'
    $form.FormBorderStyle = 'FixedDialog'
    $form.MaximizeBox = $false
    $form.MinimizeBox = $false
    $form.AutoScaleMode = 'Font'

    function Add-Label([string]$text, [int]$x, [int]$y, [int]$w, [int]$h) {
        $label = New-Object System.Windows.Forms.Label
        $label.Text = $text
        $label.SetBounds($x, $y, $w, $h)
        $form.Controls.Add($label)
        return $label
    }
    function Add-Text([string]$initial, [int]$x, [int]$y, [int]$w) {
        $control = New-Object System.Windows.Forms.TextBox
        $control.Text = $initial
        $control.SetBounds($x, $y, $w, 25)
        $form.Controls.Add($control)
        return $control
    }
    function Add-Check([string]$text, [int]$y, [bool]$checked) {
        $control = New-Object System.Windows.Forms.CheckBox
        $control.Text = $text
        $control.SetBounds(26, $y, 615, 25)
        $control.Checked = $checked
        $form.Controls.Add($control)
        return $control
    }

    $heading = Add-Label 'Automation run setup' 26 16 610 31
    $heading.Font = New-Object System.Drawing.Font('Segoe UI', 15, [System.Drawing.FontStyle]::Bold)
    $newGame = New-Object System.Windows.Forms.RadioButton
    $newGame.Text = 'New game (native setup; manual AI handover)'
    $newGame.SetBounds(26, 56, 610, 25)
    $form.Controls.Add($newGame)
    $loadGame = New-Object System.Windows.Forms.RadioButton
    $loadGame.Text = 'Load existing .sav'
    $loadGame.SetBounds(26, 84, 610, 25)
    $form.Controls.Add($loadGame)

    [void](Add-Label 'Save file (only used when loading)' 26 118 580 21)
    $source = Add-Text ([string]$options['source_save']) 26 141 546
    $browse = New-Object System.Windows.Forms.Button
    $browse.Text = 'Browse...'
    $browse.SetBounds(580, 140, 80, 29)
    $form.Controls.Add($browse)

    [void](Add-Label 'Campaign display name' 26 187 590 21)
    $campaign = Add-Text ([string]$options['campaign_name']) 26 210 634
    $campaign.MaxLength = 88
    [void](Add-Label 'Complete all-AI turns to run before next operator menu' 26 254 600 21)
    $turns = New-Object System.Windows.Forms.NumericUpDown
    $turns.Minimum = [decimal]1
    $turns.Maximum = [decimal]100000
    $turns.SetBounds(26, 277, 140, 29)
    $number = 100
    if ([int]::TryParse([string]$options['turns'], [ref]$number)) {
        if ($number -ge 1 -and $number -le 100000) { $turns.Value = [decimal]$number }
    }
    $form.Controls.Add($turns)

    $telemetry = Add-Check 'Record campaign telemetry and Council events' 323 ([string]$options['telemetry_enabled'] -eq '1')
    $notices = Add-Check 'Suppress audited noninteractive popups' 356 ([string]$options['suppress_notices'] -eq '1')
    $fast = Add-Check 'Fast AI combat / presentation' 389 ([string]$options['fast_combat'] -eq '1')

    $info = Add-Label ('Campaign logs: ' + (Join-Path $gameRoot 'campaigns')) 26 429 630 25
    $info.AutoEllipsis = $true
    [void](Add-Label 'Stage A: existing game saves/autosaves are unchanged. No ZIP/checkpoint routing yet.' 26 457 630 30)
    [void](Add-Label 'New games still require native setup and Scenario Editor handover to AI.' 26 479 630 30)

    $start = New-Object System.Windows.Forms.Button
    $start.Text = 'Launch Simulation'
    $start.SetBounds(477, 517, 183, 32)
    $form.Controls.Add($start)
    $cancel = New-Object System.Windows.Forms.Button
    $cancel.Text = 'Cancel'
    $cancel.SetBounds(366, 517, 103, 32)
    $form.Controls.Add($cancel)
    $form.CancelButton = $cancel
    $cancel.Add_Click({ $form.DialogResult = [System.Windows.Forms.DialogResult]::Cancel; $form.Close() })

    if ([string]$options['mode'] -eq 'load') { $loadGame.Checked = $true }
    else { $newGame.Checked = $true }
    $updateMode = {
        $source.Enabled = $loadGame.Checked
        $browse.Enabled = $loadGame.Checked
    }
    $newGame.Add_CheckedChanged($updateMode)
    $loadGame.Add_CheckedChanged($updateMode)
    & $updateMode

    $browse.Add_Click({
        $picker = New-Object System.Windows.Forms.OpenFileDialog
        $picker.Title = 'Select SMAC/X save to simulate'
        $picker.Filter = 'Alpha Centauri saved games (*.sav)|*.sav'
        $picker.CheckFileExists = $true
        $picker.Multiselect = $false
        $picker.InitialDirectory = $gameRoot
        if ([System.IO.Directory]::Exists((Join-Path $gameRoot 'saves'))) {
            $picker.InitialDirectory = Join-Path $gameRoot 'saves'
        }
        if ($picker.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
            $source.Text = $picker.FileName
        }
        $picker.Dispose()
    })

    $script:selectedProfile = $null
    $start.Add_Click({
        try {
            $name = $campaign.Text.Trim()
            if ($name.Length -lt 1 -or $name.Length -gt 88 -or
                $name -match '[\r\n;#]') {
                throw 'Campaign name must be 1-88 characters without newlines, ; or #.'
            }
            $mode = if ($loadGame.Checked) { 'load' } else { 'new' }
            $save = ''
            if ($mode -eq 'load') {
                $save = [System.IO.Path]::GetFullPath($source.Text.Trim())
                if (-not $save.EndsWith('.sav', [System.StringComparison]::OrdinalIgnoreCase) -or
                    -not [System.IO.File]::Exists($save) -or
                    $save -match '[\r\n]') {
                    throw 'Select an existing .sav file.'
                }
            }
            $updated = @{
                mode = $mode
                source_save = $save
                campaign_name = $name
                turns = [string][int]$turns.Value
                telemetry_enabled = if ($telemetry.Checked) { '1' } else { '0' }
                suppress_notices = if ($notices.Checked) { '1' } else { '0' }
                fast_combat = if ($fast.Checked) { '1' } else { '0' }
            }
            Save-Options $prefsPath $updated $false
            $script:selectedProfile = New-RunProfile $gameRoot $updated
            $form.DialogResult = [System.Windows.Forms.DialogResult]::OK
            $form.Close()
        } catch {
            [void][System.Windows.Forms.MessageBox]::Show(
                $_.Exception.Message, 'Automation launcher',
                [System.Windows.Forms.MessageBoxButtons]::OK,
                [System.Windows.Forms.MessageBoxIcon]::Warning)
        }
    })

    $result = $form.ShowDialog()
    $form.Dispose()
    if ($result -ne [System.Windows.Forms.DialogResult]::OK -or
        -not $script:selectedProfile) { exit 0 }

    # thinker.exe creates terranx.exe with lpEnvironment=NULL, so the
    # child inherits this same run-specific profile. No second EXE and
    # no edits to game INIs or global/user environment variables.
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = Join-Path $gameRoot 'thinker.exe'
    $startInfo.WorkingDirectory = $gameRoot
    $startInfo.UseShellExecute = $false
    $startInfo.EnvironmentVariables['THINKERNECA_AUTOSIM_PROFILE'] =
        [string]$script:selectedProfile
    $child = [System.Diagnostics.Process]::Start($startInfo)
    if (-not $child) { throw 'Could not start thinker.exe' }
    $child.Dispose()
} catch {
    try {
        [void][System.Windows.Forms.MessageBox]::Show(
            $_.Exception.Message, 'thinkerNeca Automation Launcher',
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Error)
    } catch {
        Write-Error $_.Exception.Message
    }
    exit 1
}
