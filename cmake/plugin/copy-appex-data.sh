#!/usr/bin/env bash
#
# Copy a data/ directory (or a subset of it) into an AUv3 app extension's
# Resources. Runs as an Xcode build phase (CMake POST_BUILD under the Xcode
# generator), so the destination is resolved from Xcode's TARGET_BUILD_DIR /
# UNLOCALIZED_RESOURCES_FOLDER_PATH - which follows the product into the archive
# intermediates when archiving, unlike a path baked in at configure time.
#
# Usage: copy-appex-data.sh <data dir> [subdir ...]
set -euo pipefail

DATA_DIR="$1"
shift

if [[ -z "${TARGET_BUILD_DIR:-}" || -z "${UNLOCALIZED_RESOURCES_FOLDER_PATH:-}" ]]; then
	echo "copy-appex-data.sh: TARGET_BUILD_DIR / UNLOCALIZED_RESOURCES_FOLDER_PATH are not set" >&2
	echo "  (this script must run as an Xcode build phase)" >&2
	exit 1
fi

DEST="${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/data"
mkdir -p "$DEST"

if [[ $# -eq 0 ]]; then
	rsync -a --delete "$DATA_DIR/" "$DEST/"
else
	for sub in "$@"; do
		if [ -d "$DATA_DIR/$sub" ]; then
			mkdir -p "$DEST/$sub"
			rsync -a --delete "$DATA_DIR/$sub/" "$DEST/$sub/"
		fi
	done
fi
echo "copy-appex-data.sh: assets -> $DEST"
