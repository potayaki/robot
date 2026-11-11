#requires -Version 7.0
param([switch]$Check)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$installation = & $vswhere -latest -version '[17.0,18.0)' -products '*' -property installationPath
$formatter = Join-Path $installation 'VC/Tools/Llvm/x64/bin/clang-format.exe'
if (-not (Test-Path -LiteralPath $formatter)) { throw 'Visual Studio C++ clang-format is required.' }

# 外部ライブラリと生成物を除き、ゲーム本体・自作テストだけを対象にする。
$files = @(Get-ChildItem (Join-Path $repoRoot 'DX22_01_plane') -File |
    Where-Object { $_.Extension -in '.h', '.cpp' -and $_.Name -notmatch '^(imgui|imstb|imconfig|stb_image)' })
$files += Get-ChildItem (Join-Path $repoRoot 'tests') -File | Where-Object { $_.Extension -in '.h', '.cpp' }

foreach ($file in $files) {
    if ($Check) {
        & $formatter --dry-run --Werror $file.FullName
    } else {
        # 古いShift-JISのコメントを壊さず、プロジェクトのUTF-8 BOM付きへそろえる。
        $bytes = [IO.File]::ReadAllBytes($file.FullName)
        $utf8 = [Text.UTF8Encoding]::new($false, $true)
        try { $source = $utf8.GetString($bytes).TrimStart([char]0xFEFF) }
        catch { $source = [Text.Encoding]::GetEncoding(932).GetString($bytes) }
        [IO.File]::WriteAllText($file.FullName, $source, [Text.UTF8Encoding]::new($true))
        & $formatter -i $file.FullName
    }
    if ($LASTEXITCODE -ne 0) { throw "Formatting failed: $($file.Name)" }
}
Write-Host "Checked/formatted $($files.Count) first-party C++ files."
