param(
    [Parameter(Mandatory = $true)]
    [string]$Source,

    [Parameter(Mandatory = $true)]
    [string]$Destination,

    [switch]$Preview
)

$ErrorActionPreference = "Stop"
$sourcePath = (Resolve-Path $Source).Path
$destinationPath = (Resolve-Path $Destination).Path

if (-not (Test-Path (Join-Path $sourcePath "CMakeLists.txt"))) {
    throw "Source must be the integrated project root containing CMakeLists.txt."
}

if (-not (Test-Path (Join-Path $destinationPath ".git"))) {
    throw "Destination must be the root of an existing cloned Git repository."
}

$robocopyArguments = @(
    $sourcePath,
    $destinationPath,
    "/E",
    "/R:2",
    "/W:1",
    "/XD", ".git", ".idea", "build", "build-backend",
             "cmake-build-debug", "cmake-build-release",
    "/XF", "proteus_project.txt", "recent_projects.txt"
)

if ($Preview) {
    $robocopyArguments += "/L"
}

& robocopy @robocopyArguments
$robocopyExitCode = $LASTEXITCODE

# Robocopy uses exit codes 0-7 for successful copy/difference states.
if ($robocopyExitCode -ge 8) {
    throw "Robocopy failed with exit code $robocopyExitCode."
}

if ($Preview) {
    Write-Host "Preview complete. No files were copied."
} else {
    Write-Host "Sync complete. Review the following Git changes before committing:"
    & git -C $destinationPath status --short
}
