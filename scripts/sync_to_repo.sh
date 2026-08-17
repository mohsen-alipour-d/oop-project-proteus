#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 INTEGRATED_PROJECT_DIR CLONED_REPOSITORY_DIR" >&2
    exit 2
fi

source_path="$1"
destination_path="$2"

if [[ ! -f "$source_path/CMakeLists.txt" ]]; then
    echo "Source must be the integrated project root containing CMakeLists.txt." >&2
    exit 2
fi

if [[ ! -d "$destination_path/.git" ]]; then
    echo "Destination must be the root of an existing cloned Git repository." >&2
    exit 2
fi

rsync -av \
    --exclude='.git/' \
    --exclude='.idea/' \
    --exclude='build/' \
    --exclude='build-backend/' \
    --exclude='cmake-build-debug/' \
    --exclude='cmake-build-release/' \
    --exclude='proteus_project.txt' \
    --exclude='recent_projects.txt' \
    "$source_path/" "$destination_path/"

echo "Sync complete. Review the following Git changes before committing:"
git -C "$destination_path" status --short
