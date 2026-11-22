# Builds the release artifacts: installer, portable zip, checksums, and test results.
#
#   powershell -ExecutionPolicy Bypass -File scripts\build-release.ps1
#
# Requirements: CMake 3.23+, Visual Studio 2022 with C++ tools, and Inno Setup 6 for the installer.
# Output goes to dist\. Re-running from the same commit produces the same files (see README).

[CmdletBinding()]
param(
    [string]$OutDir,
    [switch]$SkipTests,
    [switch]$SkipInstaller
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$appName = 'PS5KeyboardBridge'
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

if (-not $SkipInstaller) {
    $iscc = Find-Iscc
    $setupBase = "$appName-Setup-$version"
    & $iscc "/DAppVersion=$version" "/DStageDir=$stage" "/DOutDir=$dist" "/DSetupName=$setupBase" `
        (Join-Path $root 'installer\PS5KeyboardBridge.iss')
    if ($LASTEXITCODE) { throw 'Installer build failed.' }
    $artifacts = @((Join-Path $dist "$setupBase.exe")) + $artifacts
}

$sums = foreach ($artifact in $artifacts) {
    $hash = (Get-FileHash -Algorithm SHA256 -Path $artifact).Hash.ToLowerInvariant()
    "$hash  $(Split-Path $artifact -Leaf)"
}
$sums | Set-Content -Path (Join-Path $dist 'SHA256SUMS.txt') -Encoding ascii

Write-Host ''
Get-Content (Join-Path $dist 'SHA256SUMS.txt')
