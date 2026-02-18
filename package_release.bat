@echo off
REM Simple batch wrapper for package_release.ps1
REM Usage: package_release.bat 1.0.0

if "%~1"=="" (
    echo Usage: package_release.bat ^<version^>
    echo Example: package_release.bat 1.0.0
    exit /b 1
)

powershell -ExecutionPolicy Bypass -File "%~dp0package_release.ps1" -Version "%~1"
