$ErrorActionPreference = "Stop"

$Project = Join-Path $PSScriptRoot "Projects\MDK-ARM\atk_f407.uvprojx"
$Hex = Join-Path $PSScriptRoot "Output\atk_f407.hex"
$Log = Join-Path $PSScriptRoot "Projects\MDK-ARM\.vscode\jlink-download.log"

$JLinkCandidates = @(
    $env:JLINK_EXE,
    "D:\Keil\ARM\Segger\JLink.exe",
    "D:\Keil\Backup.001\ARM\Segger\JLink.exe",
    "C:\Program Files\SEGGER\JLink\JLink.exe",
    "C:\Program Files (x86)\SEGGER\JLink\JLink.exe"
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

if (-not (Test-Path -LiteralPath $Project)) {
    Write-Error "Project file was not found: $Project"
    exit 1
}

if (-not (Test-Path -LiteralPath $Hex)) {
    Write-Error "HEX file was not found: $Hex"
    exit 1
}

if ($JLinkCandidates) {
    $JLink = $JLinkCandidates | Select-Object -First 1
    $CmdFile = Join-Path $env:TEMP "jlink_flash_2040s_0807.jlink"
    @(
        "r",
        "h",
        "LoadFile `"$Hex`"",
        "r",
        "g",
        "q"
    ) | Set-Content -LiteralPath $CmdFile -Encoding ASCII

    Remove-Item -LiteralPath $Log -Force -ErrorAction SilentlyContinue
    Write-Host "Using J-Link: $JLink"
    & $JLink -Device STM32F407VG -If SWD -Speed 4000 -CommandFile $CmdFile 2>&1 | Tee-Object -FilePath $Log
    $ExitCode = $LASTEXITCODE

    Remove-Item -LiteralPath $CmdFile -Force -ErrorAction SilentlyContinue

    if ((Test-Path -LiteralPath $Log) -and ((Get-Content -LiteralPath $Log -Raw) -match "Error:|failed|Cannot connect|No J-Link")) {
        exit 1
    }

    exit $ExitCode
}

Write-Error "J-Link executable was not found. Install SEGGER J-Link or set JLINK_EXE to JLink.exe."
exit 1
