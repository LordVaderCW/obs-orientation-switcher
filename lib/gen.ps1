
#Requires -Version 5.1
<#
.SYNOPSIS
    Generates MSVC x64 import libraries for OBS Studio.

.DESCRIPTION
    Extracts exported symbols from obs.dll and
    obs-frontend-api.dll using dumpbin.exe.

    Creates module definition (.def) files and
    generates .lib import libraries using lib.exe.

.TARGET
    OBS Studio 32.2.1 - Windows x64

.PROJECT
    OBS Orientation Switcher
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ------------------------------------------------------------
# Project directories
# ------------------------------------------------------------

$ProjectDirectory = Split-Path -Parent $PSScriptRoot
$LibraryDirectory = Join-Path $ProjectDirectory "lib"
$ToolsDirectory = Join-Path $ProjectDirectory "tools\msvc"

$Dumpbin = Join-Path $ToolsDirectory "dumpbin.exe"
$LibTool = Join-Path $ToolsDirectory "lib.exe"

# Allow copied MSVC tools to locate their dependencies.
$env:PATH = "$ToolsDirectory;$env:PATH"

# ------------------------------------------------------------
# OBS libraries
# ------------------------------------------------------------

$Libraries = @(
    @{
        Name = "obs"
        Dll = "obs.dll"
        Output = "obs.lib"
    },
    @{
        Name = "obs-frontend-api"
        Dll = "obs-frontend-api.dll"
        Output = "obs-frontend-api.lib"
    }
)

# ------------------------------------------------------------
# Validate environment
# ------------------------------------------------------------

Write-Host ""
Write-Host "OBS Orientation Switcher - Import Library Generator" -ForegroundColor Cyan
Write-Host "Target: OBS Studio 32.2.1 / Windows x64"
Write-Host "--------------------------------------------------"

foreach ($Tool in @($Dumpbin, $LibTool)) {
    if (-not (Test-Path -LiteralPath $Tool -PathType Leaf)) {
        throw "Required MSVC tool not found: $Tool"
    }
}

# ------------------------------------------------------------
# Generate import libraries
# ------------------------------------------------------------

foreach ($Library in $Libraries) {

    $DllPath = Join-Path $LibraryDirectory $Library.Dll
    $DefPath = Join-Path $LibraryDirectory ($Library.Name + ".def")
    $OutputPath = Join-Path $LibraryDirectory $Library.Output
    $ExportsPath = Join-Path $LibraryDirectory ($Library.Name + "-exports.txt")

    Write-Host ""
    Write-Host "Processing $($Library.Dll)..." -ForegroundColor Yellow

    if (-not (Test-Path -LiteralPath $DllPath -PathType Leaf)) {
        throw "Missing OBS DLL: $DllPath"
    }

    # Extract DLL exports.
    $Exports = @(& $Dumpbin /nologo /exports $DllPath)

    if ($LASTEXITCODE -ne 0) {
        throw "DUMPBIN failed for $DllPath (exit code $LASTEXITCODE)"
    }

    [System.IO.File]::WriteAllLines(
        $ExportsPath,
        [string[]]$Exports,
        [System.Text.Encoding]::UTF8
    )

    # Parse named exports.
    $Symbols = New-Object 'System.Collections.Generic.List[string]'
    $InExportTable = $false

    foreach ($Line in $Exports) {

        if ($Line -match '^\s*ordinal\s+hint\s+RVA\s+name\s*$') {
            $InExportTable = $true
            continue
        }

        if (-not $InExportTable) {
            continue
        }

        if ($Line -match '^\s*Summary\s*$') {
            break
        }

        if ($Line -match '^\s*(\d+)\s+[0-9A-Fa-f]+\s+(?:[0-9A-Fa-f]+|forwarded)\s+(\S+)') {

            $Ordinal = $Matches[1]
            $Symbol = $Matches[2]

            # Avoid duplicate entries.
            $Entry = "    $Symbol @$Ordinal"

            if (-not $Symbols.Contains($Entry)) {
                $Symbols.Add($Entry)
            }
        }
    }

    if ($Symbols.Count -eq 0) {
        throw "No named exports found in $DllPath"
    }

    Write-Host "Found $($Symbols.Count) named exports." -ForegroundColor Green

    # Create module definition file.
    $Definition = @(
        "LIBRARY `"$($Library.Dll)`""
        "EXPORTS"
    ) + $Symbols.ToArray()

    [System.IO.File]::WriteAllLines(
        $DefPath,
        [string[]]$Definition,
        [System.Text.Encoding]::ASCII
    )

    Write-Host "Created: $DefPath"

    # Remove any previous output to prevent stale results.
    if (Test-Path -LiteralPath $OutputPath) {
        Remove-Item -LiteralPath $OutputPath -Force
    }

    # Generate x64 import library.
    Write-Host "Generating $($Library.Output)..." -ForegroundColor Cyan

    & $LibTool /nologo /machine:x64 "/def:$DefPath" "/out:$OutputPath"

    if ($LASTEXITCODE -ne 0) {
        throw "LIB.EXE failed for $OutputPath (exit code $LASTEXITCODE)"
    }

    if (-not (Test-Path -LiteralPath $OutputPath -PathType Leaf)) {
        throw "Import library was not created: $OutputPath"
    }

    $OutputFile = Get-Item -LiteralPath $OutputPath

    Write-Host "SUCCESS: $($Library.Output)" -ForegroundColor Green
    Write-Host "Size: $($OutputFile.Length) bytes"
}

Write-Host ""
Write-Host "--------------------------------------------------"
Write-Host "Both OBS import libraries generated successfully." -ForegroundColor Green
Write-Host "Output directory: $LibraryDirectory"
