# Run from the project root in Windows PowerShell:
#   .\tests\run_tests.ps1
$ErrorActionPreference = 'Stop'
$projectRoot = (Get-Location).Path
$exe = Join-Path $projectRoot 'citygrid.exe'
if (-not (Test-Path $exe)) {
    Write-Host 'citygrid.exe not found. Build first using:' -ForegroundColor Red
    Write-Host 'g++ -std=c++17 -Wall -Icore -Iindex -Iengine engine/*.cpp -o citygrid.exe'
    exit 1
}
$passed = 0
$failed = 0
Get-ChildItem (Join-Path $projectRoot 'tests') -Directory -Filter 'test*' | Sort-Object Name | ForEach-Object {
    $dir = $_.FullName
    $actual = & $exe (Join-Path $dir 'junctions.txt') (Join-Path $dir 'incidents.txt') (Join-Path $dir 'script.txt') | Out-String
    $expected = Get-Content (Join-Path $dir 'expected_output.txt') -Raw
    $normalize = { param($s) (($s -replace "`r`n", "`n" -replace "`r", "`n") -split "`n" | ForEach-Object { $_.TrimEnd(' ', "`t") }) -join "`n" }
    if ((& $normalize $actual).TrimEnd("`n") -ceq (& $normalize $expected).TrimEnd("`n")) {
        Write-Host "PASS  $($_.Name)" -ForegroundColor Green
        $script:passed++
    } else {
        Write-Host "FAIL  $($_.Name)" -ForegroundColor Red
        $script:failed++
    }
}
Write-Host "Passed: $passed   Failed: $failed"
if ($failed -gt 0) { exit 1 }
