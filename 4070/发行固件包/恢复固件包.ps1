$ErrorActionPreference = 'Stop'
$manifest = Join-Path $PSScriptRoot 'SHA256SUMS.txt'
foreach ($line in Get-Content -LiteralPath $manifest) {
    if ($line -notmatch '^([0-9a-fA-F]{64})  (.+)$') { throw "Invalid checksum line: $line" }
    $expected = $Matches[1].ToLowerInvariant()
    $name = $Matches[2]
    $output = Join-Path $PSScriptRoot $name
    if (Test-Path -LiteralPath $output) { throw "Output already exists; move it first: $output" }
    $parts = @(Get-ChildItem -LiteralPath $PSScriptRoot -File -Filter ($name + '.part*') | Sort-Object Name)
    if ($parts.Count -lt 2) { throw "Missing archive parts for $name" }
    $stream = [System.IO.File]::Create($output)
    try {
        foreach ($part in $parts) {
            if ($part.Length -ge 100000000) { throw "Part exceeds GitHub's regular file limit: $($part.Name)" }
            $input = [System.IO.File]::OpenRead($part.FullName)
            try { $input.CopyTo($stream) } finally { $input.Dispose() }
        }
    } finally { $stream.Dispose() }
    $actual = (Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $expected) { throw "SHA-256 mismatch for $name; expected $expected, got $actual" }
    Write-Host "Restored and verified $name"
}