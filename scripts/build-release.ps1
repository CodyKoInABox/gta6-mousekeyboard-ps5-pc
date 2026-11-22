# Builds the release artifacts: installer, portable zip, checksums, and test results.
#
#   powershell -ExecutionPolicy Bypass -File scripts\build-release.ps1
#
# Requirements: CMake 3.23+, Visual Studio 2022 with C++ tools, and Inno Setup 6 for the installer.
# Output goes to dist\. Re-running from the same commit produces the same files (see README).
#
# Smoke test: the installer is installed silently into a temp folder and the installed exe runs
# `--self-test`; the portable zip is extracted and tested the same way. Both are uninstalled or
# removed afterwards. The build fails on any error. The smoke test refuses to run if
# PS5 Keyboard Bridge is already installed for this user, because the installer would overwrite
# its uninstall entry and Start menu shortcuts. Use -SkipSmokeTest to bypass it.

[CmdletBinding()]
param(
    [string]$OutDir,
    [switch]$SkipTests,
    [switch]$SkipInstaller,
    [switch]$SkipSmokeTest
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$appName = 'PS5KeyboardBridge'
$displayName = 'PS5 Keyboard Bridge'
$installAppId = '{7B1E4C2A-5D3F-4A8B-9E6C-2F1D0A4B7C93}'
if (-not $OutDir) { $OutDir = Join-Path $root 'dist' }

function Get-ProjectVersion {
    $cmake = Get-Content -Raw -Path (Join-Path $root 'CMakeLists.txt')
    if ($cmake -notmatch 'project\(PS5KeyboardBridge VERSION (\d+\.\d+\.\d+)') {
        throw 'Could not read the version from CMakeLists.txt.'
    }
    return $Matches[1]
}

function Find-Iscc {
    $candidates = @(
        (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
        (Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe')
    )
    foreach ($path in $candidates) {
        if (Test-Path $path) { return $path }
    }
    throw 'Inno Setup 6 (ISCC.exe) not found. Install it, or run with -SkipInstaller for the portable build only.'
}

# Zip entries get a fixed timestamp and sorted order, so the same inputs give the same bytes.
function New-DeterministicZip {
    param([string]$SourceDir, [string]$ZipPath)
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
    $stamp = [DateTimeOffset]::new(2026, 1, 1, 0, 0, 0, [TimeSpan]::Zero)
    $base = (Resolve-Path $SourceDir).Path
    $files = Get-ChildItem -Path $base -Recurse -File | Sort-Object FullName
    $archive = [System.IO.Compression.ZipFile]::Open($ZipPath, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($file in $files) {
            $entryName = $file.FullName.Substring($base.Length + 1).Replace('\', '/')
            $entry = $archive.CreateEntry("$appName/$entryName", [System.IO.Compression.CompressionLevel]::Optimal)
            $entry.LastWriteTime = $stamp
            $in = [System.IO.File]::OpenRead($file.FullName)
            try {
                $out = $entry.Open()
                try { $in.CopyTo($out) } finally { $out.Dispose() }
            } finally { $in.Dispose() }
        }
    } finally {
        $archive.Dispose()
    }
}

# Exit codes from `PS5KeyboardBridge.exe --self-test`; keep in sync with src\app\self_test.h.
$selfTestFailures = @{
    2 = 'Windows version is not supported'
    3 = 'a bundled file is missing next to the exe'
    4 = 'the main window could not be created'
}

function Assert-NoExistingInstall {
    $found = @()
    if (Test-Path "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\${installAppId}_is1") {
        $found += 'uninstall entry'
    }
    if (Test-Path (Join-Path ([Environment]::GetFolderPath('Programs')) $displayName)) {
        $found += 'Start menu folder'
    }
    if (Test-Path (Join-Path ([Environment]::GetFolderPath('Desktop')) "$displayName.lnk")) {
        $found += 'desktop shortcut'
    }
    if (Test-Path (Join-Path $env:LOCALAPPDATA "Programs\$appName")) {
        $found += 'install folder'
    }
    if ($found.Count -gt 0) {
        throw "Smoke test refused: $displayName is already installed for this user ($($found -join ', ')). Uninstall it first, or run with -SkipSmokeTest."
    }
}

function Invoke-SelfTest {
    param([string]$Exe, [string]$Label)
    $process = Start-Process -FilePath $Exe -ArgumentList '--self-test' -PassThru -WindowStyle Hidden
    if (-not $process.WaitForExit(60000)) {
        $process.Kill()
        throw "$Label self-test did not exit within 60 seconds."
    }
    if ($process.ExitCode -ne 0) {
        $reason = $selfTestFailures[$process.ExitCode]
        if (-not $reason) { $reason = 'unexpected exit code' }
        throw "$Label self-test failed with exit code $($process.ExitCode): $reason."
    }
    Write-Host "$Label self-test passed."
}

function Wait-Until {
    param([scriptblock]$Condition, [string]$Failure)
    $deadline = (Get-Date).AddSeconds(30)
    while (-not (& $Condition)) {
        if ((Get-Date) -gt $deadline) { throw $Failure }
        Start-Sleep -Milliseconds 500
    }
}

function Invoke-InstallerSmokeTest {
    param([string]$Installer)
    Assert-NoExistingInstall
    $work = Join-Path ([IO.Path]::GetTempPath()) "pskb-smoke-$([guid]::NewGuid().ToString('N'))"
    $appDir = Join-Path $work 'app'
    $exe = Join-Path $appDir "$appName.exe"
    $uninstaller = Join-Path $appDir 'unins000.exe'
    $uninstallKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\${installAppId}_is1"
    $startMenu = Join-Path ([Environment]::GetFolderPath('Programs')) $displayName
    $desktopShortcut = Join-Path ([Environment]::GetFolderPath('Desktop')) "$displayName.lnk"
    New-Item -ItemType Directory -Path $work | Out-Null
    try {
        $install = Start-Process -FilePath $Installer -Wait -PassThru -ArgumentList @(
            '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-',
            '/MERGETASKS="!desktopicon"', "/DIR=`"$appDir`""
        )
        if ($install.ExitCode -ne 0) { throw "Silent install failed with exit code $($install.ExitCode)." }
        foreach ($name in @("$appName.exe", 'ps5kb-cli.exe', 'unins000.exe')) {
            if (-not (Test-Path (Join-Path $appDir $name))) { throw "Installed file missing: $name" }
        }
        if (Test-Path $desktopShortcut) { throw 'Installer created a desktop shortcut although the desktop task was deselected.' }
        Invoke-SelfTest -Exe $exe -Label 'Installed build'
    } finally {
        if (Test-Path $uninstaller) {
            $uninstall = Start-Process -FilePath $uninstaller -Wait -PassThru -ArgumentList '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART'
            if ($uninstall.ExitCode -ne 0) { throw "Silent uninstall failed with exit code $($uninstall.ExitCode)." }
            Wait-Until -Condition { -not (Test-Path $exe) } -Failure 'Uninstall left PS5KeyboardBridge.exe behind.'
            Wait-Until -Condition { -not (Test-Path $uninstallKey) } -Failure 'Uninstall left its uninstall entry behind.'
            Wait-Until -Condition { -not (Test-Path $startMenu) } -Failure 'Uninstall left the Start menu folder behind.'
        }
        if (Test-Path $work) { Remove-Item $work -Recurse -Force }
    }
    Write-Host 'Installer smoke test passed (silent install, self-test, silent uninstall).'
}

function Invoke-PortableSmokeTest {
    param([string]$Zip)
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $work = Join-Path ([IO.Path]::GetTempPath()) "pskb-smoke-$([guid]::NewGuid().ToString('N'))"
    try {
        [System.IO.Compression.ZipFile]::ExtractToDirectory($Zip, $work)
        Invoke-SelfTest -Exe (Join-Path $work "$appName\$appName.exe") -Label 'Portable build'
    } finally {
        if (Test-Path $work) { Remove-Item $work -Recurse -Force }
    }
}

$version = Get-ProjectVersion
$dist = (New-Item -ItemType Directory -Force -Path $OutDir).FullName
$buildDir = Join-Path $root 'build-release'
$stage = Join-Path $dist "stage\$appName"

Write-Host "Version $version"

if (Test-Path $buildDir) { Remove-Item $buildDir -Recurse -Force }
cmake -S $root -B $buildDir -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE) { throw 'CMake configure failed.' }
cmake --build $buildDir --config Release --target pskb_app pskb_cli pskb_tests
if ($LASTEXITCODE) { throw 'Build failed.' }

$testResults = Join-Path $dist 'test-results.txt'
if (-not $SkipTests) {
    & (Join-Path $buildDir 'Release\pskb_tests.exe') 2>&1 | Tee-Object -FilePath $testResults
    if ($LASTEXITCODE) { throw 'Automated tests failed. See test-results.txt.' }
}

if (Test-Path (Join-Path $dist 'stage')) { Remove-Item (Join-Path $dist 'stage') -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item (Join-Path $buildDir "Release\$appName.exe") $stage
Copy-Item (Join-Path $buildDir 'Release\ps5kb-cli.exe') $stage
Copy-Item (Join-Path $root 'LICENSE') (Join-Path $stage 'LICENSE.txt')
Copy-Item (Join-Path $root 'THIRD_PARTY_NOTICES.md') $stage
Copy-Item (Join-Path $root 'docs\guia-configuracao.md') $stage
Copy-Item (Join-Path $root 'docs\solucao-de-problemas.md') $stage
# Inno Setup records each file's modification time in the installer, so pin them.
$fixedStamp = [DateTime]::new(2026, 1, 1, 0, 0, 0, [DateTimeKind]::Utc)
Get-ChildItem -Path $stage -Recurse -File | ForEach-Object { $_.LastWriteTimeUtc = $fixedStamp }

$portable = Join-Path $dist "$appName-$version-portable.zip"
New-DeterministicZip -SourceDir $stage -ZipPath $portable
$artifacts = @($portable)
$installer = $null

if (-not $SkipInstaller) {
    $iscc = Find-Iscc
    $setupBase = "$appName-Setup-$version"
    & $iscc "/DAppVersion=$version" "/DStageDir=$stage" "/DOutDir=$dist" "/DSetupName=$setupBase" `
        (Join-Path $root 'installer\PS5KeyboardBridge.iss')
    if ($LASTEXITCODE) { throw 'Installer build failed.' }
    $installer = Join-Path $dist "$setupBase.exe"
    $artifacts = @($installer) + $artifacts
}

if (-not $SkipSmokeTest) {
    if ($installer) { Invoke-InstallerSmokeTest -Installer $installer }
    Invoke-PortableSmokeTest -Zip $portable
}

$sums = foreach ($artifact in $artifacts) {
    $hash = (Get-FileHash -Algorithm SHA256 -Path $artifact).Hash.ToLowerInvariant()
    "$hash  $(Split-Path $artifact -Leaf)"
}
$sums | Set-Content -Path (Join-Path $dist 'SHA256SUMS.txt') -Encoding ascii

Write-Host ''
Get-Content (Join-Path $dist 'SHA256SUMS.txt')
