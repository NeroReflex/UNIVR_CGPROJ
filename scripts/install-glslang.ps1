<#
Non-interactive installer for glslangValidator via Chocolatey.
Intended for CI (GitHub Actions) or running on Windows machines.
Usage:
  PowerShell -NoProfile -ExecutionPolicy Bypass -File .\scripts\install-glslang.ps1
Exits with 0 if `glslangValidator` is available, non-zero otherwise.
#>

$ErrorActionPreference = 'Stop'

function Write-Info {
    Write-Host "[install-glslang] $args" -ForegroundColor Cyan
}

Write-Info "Checking for choco..."
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Info "Chocolatey not found. Installing Chocolatey non-interactively..."
    Set-ExecutionPolicy Bypass -Scope Process -Force
    $installScript = 'https://community.chocolatey.org/install.ps1'
    try {
        iex ((New-Object System.Net.WebClient).DownloadString($installScript))
    } catch {
        Write-Error "Failed to download/install Chocolatey: $_"
        exit 2
    }
} else {
    Write-Info "Chocolatey already present."
}

# Ensure choco won't prompt or show progress
Write-Info "Disabling download progress and ensuring non-interactive flags..."
choco feature disable -n showDownloadProgress | Out-Null 2>$null

# Install glslang non-interactively
Write-Info "Installing glslang (glslangValidator) via Chocolatey..."
try {
    choco install glslang -y --no-progress --limit-output | Out-Null
} catch {
    Write-Error "choco install failed: $_"
    exit 3
}

# Try to refresh environment (refreshenv is provided by chocolately package)
try {
    Write-Info "Refreshing environment variables..."
    refreshenv | Out-Null
} catch {
    # refreshenv may not exist; ignore
}

# Verify installation
Write-Info "Verifying glslangValidator is available..."
if (Get-Command glslangValidator -ErrorAction SilentlyContinue) {
    $ver = & glslangValidator --version 2>&1
    Write-Host "glslangValidator found:" $ver
    exit 0
} else {
    # Try common chocolatey bin location
    $paths = @(
        "$env:ChocolateyInstall\bin",
        'C:\ProgramData\chocolatey\bin',
        'C:\Program Files\glslang\bin'
    )
    $found = $false
    foreach ($p in $paths) {
        if (Test-Path $p) {
            $exe = Join-Path $p 'glslangValidator.exe'
            if (Test-Path $exe) {
                Write-Host "Found glslangValidator at $exe"
                & $exe --version
                $found = $true
                break
            }
        }
    }
    if ($found) { exit 0 }
    Write-Error "glslangValidator not found after installation."
    exit 4
}
