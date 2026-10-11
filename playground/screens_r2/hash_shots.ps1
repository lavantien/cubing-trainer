param([string]$OutFile)

$root = "C:\Users\lavantien\dev\github\cubing-trainer"
$lines = Get-ChildItem -LiteralPath (Join-Path $root "docs\img\screens") -Filter *.png |
    Sort-Object Name |
    ForEach-Object {
        $h = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower()
        "$h $($_.Name)"
    }
Set-Content -LiteralPath (Join-Path $root $OutFile) -Value $lines -Encoding ascii
Write-Output $lines
