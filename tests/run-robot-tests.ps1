#requires -Version 7.0
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$installation = & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio 2022 with the v143 C++ toolset is required.' }
$vcvars = Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
$assimpRoot = @('C:\assimp\5.2.5', 'C:\assimp\assimp\5.2.5') |
    Where-Object { Test-Path "$_/include/assimp/Importer.hpp" } | Select-Object -First 1
if (-not $assimpRoot) { throw 'Assimp 5.2.5 is missing.' }

$cases = @(
    @{ Name = 'RobotAnimationTests'; Sources = @('RobotAnimationRig.cpp') },
    @{ Name = 'RobotSpiderIKTests'; Sources = @('RobotAnimationRig.cpp', 'RobotSpiderIK.cpp', 'RobotDebris.cpp') },
    @{ Name = 'RobotDebrisTests'; Sources = @('RobotAnimationRig.cpp', 'RobotDebris.cpp') }
)
$originalPath = $env:PATH
try {
    $env:PATH = "$repoRoot\DX22_01_plane;$assimpRoot\bin;$originalPath"
    foreach ($case in $cases) {
        # Never put standalone objects in the application's intermediate directory.
        $output = Join-Path $PSScriptRoot "obj/v143/x64/Debug/$($case.Name)"
        New-Item -ItemType Directory -Force -Path $output | Out-Null
        $sources = @((Join-Path $PSScriptRoot "$($case.Name).cpp"))
        $sources += $case.Sources | ForEach-Object { Join-Path $repoRoot "DX22_01_plane/$_" }
        # Relative source names keep non-ASCII workspace paths out of cmd.exe's batch parser.
        $arguments = ($sources | ForEach-Object { '"' + [IO.Path]::GetRelativePath($output, $_) + '"' }) -join ' '
        $batch = @"
@echo off
call "$vcvars" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /MTd /std:c++20 /utf-8 /I "$assimpRoot\include" $arguments /Fe:$($case.Name).exe /link /LIBPATH:"$assimpRoot\lib" assimp-vc143-mtd.lib
exit /b %errorlevel%
"@
        $batchPath = Join-Path $output 'build.cmd'
        [IO.File]::WriteAllText($batchPath, $batch, [Text.Encoding]::ASCII)
        Push-Location $output
        try {
            & cmd.exe /d /c build.cmd
            if ($LASTEXITCODE -ne 0) { throw "$($case.Name) compilation failed." }
        } finally { Pop-Location }
        Push-Location $repoRoot
        try {
            & (Join-Path $output "$($case.Name).exe")
            if ($LASTEXITCODE -ne 0) { throw "$($case.Name) failed." }
        } finally { Pop-Location }
    }
} finally { $env:PATH = $originalPath }
