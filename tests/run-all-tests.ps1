#requires -Version 7.0
param([string[]]$Suites = @())
# アプリの中間ファイルと混在させず、既存の単体テストをまとめて検証する。
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$installation = & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio 2022 with the v143 C++ toolset is required.' }
$vcvars = Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
$assimpRoot = @('C:\assimp\5.2.5', 'C:\assimp\assimp\5.2.5') |
    Where-Object { Test-Path "$_/include/assimp/Importer.hpp" } | Select-Object -First 1
if (-not $assimpRoot) { throw 'Assimp 5.2.5 is missing.' }
$directXRoot = 'C:\directxtk'
if (-not (Test-Path "$directXRoot/include/SimpleMath.h")) { throw 'DirectXTK is missing.' }

# ヘッダーだけのテストはSourcesを空にする。描画テストはWARPでGPU処理も検証する。
$cases = @(
    @{ Name = 'GameTimeTests'; Sources = @() },
    @{ Name = 'SkyCycleTests'; Sources = @() },
    @{ Name = 'BossBattleTests'; Sources = @() },
    @{ Name = 'BoarChargeTests'; Sources = @() },
    @{ Name = 'MushroomFuseTests'; Sources = @() },
    @{ Name = 'BulletHomingTests'; Sources = @() },
    @{ Name = 'EyeTurretTests'; Sources = @() },
    @{ Name = 'MissileFlightTests'; Sources = @('Bezier.cpp') },
    @{ Name = 'SurvivorProgressionTests'; Sources = @('SurvivorProgression.cpp') },
    @{ Name = 'BoarAnimationTests'; Sources = @('BoarAnimationRig.cpp') },
    @{ Name = 'RobotAnimationTests'; Sources = @('RobotAnimationRig.cpp') },
    @{ Name = 'RobotSpiderIKTests'; Sources = @('RobotAnimationRig.cpp', 'RobotSpiderIK.cpp', 'RobotDebris.cpp') },
    @{ Name = 'RobotDebrisTests'; Sources = @('RobotAnimationRig.cpp', 'RobotDebris.cpp') },
    @{ Name = 'LowHealthGlitchTests'; Sources = @('LowHealthGlitch.cpp') }
)
$originalPath = $env:PATH
try {
    $env:PATH = "$repoRoot\DX22_01_plane;$assimpRoot\bin;$originalPath"
    foreach ($case in $cases) {
        if ($Suites.Count -gt 0 -and $case.Name -notin $Suites) { continue }
        # アプリ用objには書き込まない。構造体の大きさが違う古いobjとの混在を防ぐ。
        $output = Join-Path $PSScriptRoot "obj/v143/x64/Debug/$($case.Name)"
        New-Item -ItemType Directory -Force -Path $output | Out-Null
        $sources = @((Join-Path $PSScriptRoot "$($case.Name).cpp"))
        $sources += $case.Sources | ForEach-Object { Join-Path $repoRoot "DX22_01_plane/$_" }
        $arguments = ($sources | ForEach-Object { '"' + [IO.Path]::GetRelativePath($output, $_) + '"' }) -join ' '
        # 日本語を含むワークスペース名がcmdのコードページに影響されないよう相対パスを使う。
        $batch = @"
@echo off
call "$vcvars" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /MTd /std:c++20 /utf-8 /D_DEBUG /I "$assimpRoot\include" /I "$directXRoot\include" $arguments /Fe:$($case.Name).exe /link /LIBPATH:"$assimpRoot\lib" /LIBPATH:"$directXRoot\lib\x64\Debug" assimp-vc143-mtd.lib DirectXTK.lib d3d11.lib d3dcompiler.lib dxguid.lib
exit /b %errorlevel%
"@
        [IO.File]::WriteAllText((Join-Path $output 'build.cmd'), $batch, [Text.Encoding]::ASCII)
        Push-Location $output
        try {
            & cmd.exe /d /c build.cmd
            if ($LASTEXITCODE -ne 0) { throw "$($case.Name) compilation failed." }
        } finally { Pop-Location }
        # この描画テストはゲームと同じ相対パスでHLSLを読み込む。
        $runDirectory = if ($case.Name -eq 'LowHealthGlitchTests') {
            Join-Path $repoRoot 'DX22_01_plane'
        } else { $repoRoot }
        Push-Location $runDirectory
        try {
            & (Join-Path $output "$($case.Name).exe")
            if ($LASTEXITCODE -ne 0) { throw "$($case.Name) failed." }
        } finally { Pop-Location }
    }
    Write-Host 'All selected test suites passed.'
} finally { $env:PATH = $originalPath }
