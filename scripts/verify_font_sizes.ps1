# Verify right-cluster LVGL fonts match UI expectations (no device flash).
$ErrorActionPreference = "Stop"
$FontsDir = Join-Path (Split-Path -Parent $PSScriptRoot) "main\ui\fonts"

function Get-FontMetrics {
    param([string]$Path)
    $t = Get-Content -Raw -Path $Path
    $name = [System.IO.Path]::GetFileNameWithoutExtension($Path)
    $nominal = if ($t -match '(?m)^\* Size: (\d+) px') { [int]$Matches[1] }
               elseif ($t -match '--size (\d+)') { [int]$Matches[1] }
               elseif ($t -match 'font, (\d+)px') { [int]$Matches[1] }
               else { $null }
    $lh = if ($t -match '\.line_height\s*=\s*(\d+)') { [int]$Matches[1] } else { $null }
    $bf = if ($t -match '\.bitmap_format\s*=\s*(\d+)') { [int]$Matches[1] } else { $null }
    $boxHeights = [regex]::Matches($t, '\.box_h\s*=\s*(\d+)') | ForEach-Object { [int]$_.Groups[1].Value } | Where-Object { $_ -gt 0 }
    $maxBox = if ($boxHeights.Count) { ($boxHeights | Measure-Object -Maximum).Maximum } else { $null }
    [pscustomobject]@{ Name = $name; NominalPx = $nominal; LineHeight = $lh; MaxBoxH = $maxBox; BitmapFormat = $bf }
}

$expected = @{
    aerospace_56 = @{ NominalPx = 56; UsedIn = "ui_lambda.c value" }
    aerospace_22 = @{ NominalPx = 22; UsedIn = "ui_bar_gauges.c values" }
    racehead_19    = @{ NominalPx = 19; UsedIn = "ui_bar_gauges.c labels" }
    racehead_17    = @{ NominalPx = 17; UsedIn = "ui_warning.c lines" }
    racehead_14    = @{ NominalPx = 14; UsedIn = "ui_menu_popup.c, ui_warning title" }
    racehead_12    = @{ NominalPx = 12; UsedIn = "ui_menu_popup.c title/footer" }
}

$fail = $false
Write-Host "Right cluster font verification"
Write-Host ""

foreach ($key in $expected.Keys | Sort-Object) {
    $path = Join-Path $FontsDir "$key.c"
    if (-not (Test-Path $path)) {
        Write-Host "FAIL  missing $key.c"
        $fail = $true
        continue
    }
    $m = Get-FontMetrics $path
    $exp = $expected[$key]
    $okNom = ($m.NominalPx -eq $exp.NominalPx)
    if (-not $okNom) {
        Write-Host "FAIL  $($m.Name): nominal/line_height $($m.NominalPx)/$($m.LineHeight) expected $($exp.NominalPx) px ($($exp.UsedIn))"
        $fail = $true
    } else {
        Write-Host "OK    $($m.Name): $($exp.NominalPx) px  line_height=$($m.LineHeight)  max_glyph_h=$($m.MaxBoxH)  format=$($m.BitmapFormat)  -> $($exp.UsedIn)"
    }
}

# convert.sh must list every shipped font
$convert = Get-Content -Raw (Join-Path $FontsDir "convert.sh")
$missingInScript = @()
foreach ($key in $expected.Keys) {
    if ($convert -notmatch "run\s+$key\s") { $missingInScript += $key }
}
if ($missingInScript.Count) {
    Write-Host ""
    Write-Host "WARN  convert.sh missing regenerate rules for: $($missingInScript -join ', ')"
}
if ($convert -match 'OUT_DIR/\$name\.c' -or $convert -match 'OUT_DIR/\.c') {
    if ($convert -notmatch '\$\{OUT_DIR\}/\$\{name\}\.c') {
        Write-Host "FAIL  convert.sh output path must be `${OUT_DIR}/`${name}.c"
        $fail = $true
    }
}

# racehead_22 built but unused
$cmake = Get-Content -Raw (Join-Path (Split-Path -Parent $PSScriptRoot) "main\CMakeLists.txt")
if ($cmake -match 'racehead_22\.c') {
    $ui = Get-ChildItem (Join-Path (Split-Path -Parent $PSScriptRoot) "main\ui\*.c") -Recurse | Get-Content -Raw
    if ($ui -notmatch 'racehead_22') {
        Write-Host "WARN  racehead_22.c is compiled but not referenced in UI (dead weight)"
    }
}

Write-Host ""
if ($fail) { throw "Font verification failed" }
Write-Host "PASS  all UI fonts present at expected nominal sizes."
