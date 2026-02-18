<#
.SYNOPSIS
    Packages MaterialTagPlugin for GitHub release distribution.

.DESCRIPTION
    Creates release ZIP files containing:
    - Source distribution (for building from source)
    - Binary distribution (pre-built for UE 5.3)

.PARAMETER Version
    Version string for the release (e.g., "1.0.0")

.PARAMETER OutputDir
    Output directory for the ZIP files. Defaults to "Releases" in plugin root.

.PARAMETER SkipBinaries
    Skip creating binary distribution (source-only release)

.EXAMPLE
    .\package_release.ps1 -Version "1.0.0"
    
.EXAMPLE
    .\package_release.ps1 -Version "1.0.0" -OutputDir "C:\Releases"
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$Version,
    
    [string]$OutputDir = "",
    
    [switch]$SkipBinaries
)

$ErrorActionPreference = "Stop"

# Get script directory (plugin root)
$PluginRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

if ([string]::IsNullOrEmpty($OutputDir)) {
    $OutputDir = Join-Path $PluginRoot "Releases"
}

# Create output directory
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "MaterialTagPlugin Release Packager" -ForegroundColor Cyan
Write-Host "Version: $Version" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Define what to include
$SourceFiles = @(
    "MaterialTagPlugin.uplugin",
    "README.md",
    "LICENSE",
    "Source"
)

$BinaryFiles = @(
    "MaterialTagPlugin.uplugin",
    "README.md",
    "LICENSE",
    "Source",
    "Binaries"
)

# Excluded patterns
$ExcludePatterns = @(
    "*.pdb",
    "Intermediate",
    "Releases",
    ".git",
    ".gitignore",
    "*.zip",
    "package_release.ps1"
)

function Copy-FilteredItems {
    param(
        [string]$Source,
        [string]$Destination,
        [string[]]$Items,
        [string[]]$Excludes
    )
    
    foreach ($item in $Items) {
        $sourcePath = Join-Path $Source $item
        $destPath = Join-Path $Destination $item
        
        if (Test-Path $sourcePath) {
            if ((Get-Item $sourcePath).PSIsContainer) {
                # Directory - copy recursively with exclusions
                $destDir = New-Item -ItemType Directory -Path $destPath -Force
                
                Get-ChildItem -Path $sourcePath -Recurse | ForEach-Object {
                    $relativePath = $_.FullName.Substring($sourcePath.Length + 1)
                    $skip = $false
                    
                    foreach ($exclude in $Excludes) {
                        if ($_.Name -like $exclude -or $relativePath -like "*$exclude*") {
                            $skip = $true
                            break
                        }
                    }
                    
                    if (-not $skip) {
                        $targetPath = Join-Path $destPath $relativePath
                        if ($_.PSIsContainer) {
                            if (-not (Test-Path $targetPath)) {
                                New-Item -ItemType Directory -Path $targetPath -Force | Out-Null
                            }
                        } else {
                            $targetDir = Split-Path -Parent $targetPath
                            if (-not (Test-Path $targetDir)) {
                                New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
                            }
                            Copy-Item -Path $_.FullName -Destination $targetPath -Force
                        }
                    }
                }
            } else {
                # File - copy directly
                Copy-Item -Path $sourcePath -Destination $destPath -Force
            }
        } else {
            Write-Warning "Item not found: $sourcePath"
        }
    }
}

function New-ReleaseZip {
    param(
        [string]$Name,
        [string[]]$Items,
        [string[]]$Excludes
    )
    
    $zipName = "MaterialTagPlugin-$Version-$Name.zip"
    $zipPath = Join-Path $OutputDir $zipName
    $tempDir = Join-Path $env:TEMP "MaterialTagPlugin_$Name_$(Get-Random)"
    $packageDir = Join-Path $tempDir "MaterialTagPlugin"
    
    Write-Host "Creating $Name distribution..." -ForegroundColor Yellow
    
    try {
        # Create temp directory structure
        New-Item -ItemType Directory -Path $packageDir -Force | Out-Null
        
        # Copy files
        Copy-FilteredItems -Source $PluginRoot -Destination $packageDir -Items $Items -Excludes $Excludes
        
        # Remove existing zip if present
        if (Test-Path $zipPath) {
            Remove-Item $zipPath -Force
        }
        
        # Create ZIP
        Compress-Archive -Path $packageDir -DestinationPath $zipPath -CompressionLevel Optimal
        
        $size = (Get-Item $zipPath).Length / 1MB
        Write-Host "  Created: $zipName ($([math]::Round($size, 2)) MB)" -ForegroundColor Green
        
        return $zipPath
    }
    finally {
        # Cleanup temp directory
        if (Test-Path $tempDir) {
            Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

# Verify required files exist
Write-Host "Verifying plugin structure..." -ForegroundColor Yellow
$requiredFiles = @("MaterialTagPlugin.uplugin", "Source", "README.md", "LICENSE")
foreach ($file in $requiredFiles) {
    $path = Join-Path $PluginRoot $file
    if (-not (Test-Path $path)) {
        Write-Error "Required file/folder missing: $file"
        exit 1
    }
}
Write-Host "  Plugin structure OK" -ForegroundColor Green
Write-Host ""

# Create source distribution
$sourceZip = New-ReleaseZip -Name "Source" -Items $SourceFiles -Excludes $ExcludePatterns

# Create binary distribution (if binaries exist and not skipped)
$binariesPath = Join-Path $PluginRoot "Binaries"
if (-not $SkipBinaries -and (Test-Path $binariesPath)) {
    $binaryZip = New-ReleaseZip -Name "Win64" -Items $BinaryFiles -Excludes $ExcludePatterns
} elseif (-not $SkipBinaries) {
    Write-Warning "Binaries folder not found - skipping binary distribution"
    Write-Warning "Build the plugin first to create binary releases"
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Release packaging complete!" -ForegroundColor Green
Write-Host "Output directory: $OutputDir" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Release files:" -ForegroundColor Yellow
Get-ChildItem -Path $OutputDir -Filter "MaterialTagPlugin-$Version-*.zip" | ForEach-Object {
    Write-Host "  - $($_.Name)" -ForegroundColor White
}
Write-Host ""
Write-Host "Upload these files to your GitHub release." -ForegroundColor Gray
