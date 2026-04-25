#!/bin/bash
set -euo pipefail

echo "Downloading latest EVEDBTool..."

if ! command -v curl >/dev/null 2>&1; then
    echo "curl is required but was not found."
    exit 1
fi

ARCH="$(uname -m)"
API_URL="https://api.github.com/repos/EvEmu-Project/EVEDBTool/releases/latest"

case "$ARCH" in
    aarch64|arm64)
        echo "Using aarch64 build..."
        ASSET_FILTER='evedb_aarch64'
        ;;
    x86_64|amd64)
        echo "Using x86_64 build..."
        ASSET_FILTER='evedbtool'
        ;;
    *)
        echo "Unsupported architecture: $ARCH"
        exit 1
        ;;
esac

DOWNLOAD_URL="$(curl -fsSL "$API_URL" \
    | grep browser_download_url \
    | grep "$ASSET_FILTER" \
    | grep -v '\.exe"' \
    | cut -d '"' -f 4 \
    | head -n 1)"

if [ -z "$DOWNLOAD_URL" ]; then
    echo "Failed to resolve EVEDBTool download URL for architecture: $ARCH"
    exit 1
fi

TMP_OUT="evedbtool.tmp"
curl -fsSL "$DOWNLOAD_URL" -o "$TMP_OUT"
install -m 0755 "$TMP_OUT" evedbtool
rm -f "$TMP_OUT"

if [ ! -x evedbtool ]; then
    echo "EVEDBTool download completed but executable was not created."
    exit 1
fi

echo "EVEDBTool downloaded to $(pwd)/evedbtool"
