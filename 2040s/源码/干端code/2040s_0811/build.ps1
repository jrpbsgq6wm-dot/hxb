$ErrorActionPreference = "Stop"

$Project = Join-Path $PSScriptRoot "Projects\MDK-ARM\atk_f407.uvprojx"
$Target = "lwIP"
$Log = Join-Path $PSScriptRoot "Projects\MDK-ARM\.vscode\uv4-build.log"

$Candidates = @(
    $env:KEIL_UV4,
    "C:\Keil_v5\UV4\UV4.exe",
    "C:\Program Files\Keil_v5\UV4\UV4.exe",
    "C:\Program Files (x86)\Keil_v5\UV4\UV4.exe",
    "D:\Keil_v5\UV4\UV4.exe",
    "D:\Keil\UV4\UV4.exe",
    "E:\Keil_v5\UV4\UV4.exe"
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

if (-not $Candidates) {
    Write-Error "Keil UV4.exe was not found. Install Keil MDK v5, or set KEIL_UV4 to the full UV4.exe path."
    exit 1
}

if (-not (Test-Path -LiteralPath $Project)) {
    Write-Error "Project file was not found: $Project"
    exit 1
}

$Uv4 = $Candidates | Select-Object -First 1
Write-Host "Using Keil: $Uv4"
Write-Host "Building target: $Target"

Remove-Item -LiteralPath $Log -Force -ErrorAction SilentlyContinue
& $Uv4 -b $Project -j0 -t $Target -o $Log
$ExitCode = $LASTEXITCODE

if (Test-Path -LiteralPath $Log) {
    Get-Content -LiteralPath $Log
}

exit $ExitCode
