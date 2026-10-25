#!/usr/bin/env bash
#
# Download and install the correct pyspla wheel for the current platform.
#
# Detects OS + architecture via Python, resolves the matching wheel from a
# GitHub Release, downloads it to a temp directory and installs with pip.
#
# Usage:
#   curl -sL https://raw.githubusercontent.com/<repo>/<branch>/install.sh | bash
# or:
#   chmod +x install.sh && ./install.sh
#
# Override defaults with environment variables:
#   SPLA_INSTALL_VERSION   wheel version to install  (default 0.0.5)
#   SPLA_INSTALL_TAG       GitHub Release tag        (default v0.0.5-<branch>)
#   SPLA_INSTALL_REPO      owner/repo               (default read from this file)

set -euo pipefail

REPO="${SPLA_INSTALL_REPO:-Gitubrr/spla}"
VERSION="${SPLA_INSTALL_VERSION:-0.0.5}"

# Derive a default tag if not provided: use branch-aware pre-release tag.
DEFAULT_TAG="v${VERSION}-whls"
TAG="${SPLA_INSTALL_TAG:-${DEFAULT_TAG}}"

# ------------------------------------------------------------------ detect platform
read -r OS ARCH <<< "$(python3 -c "
import platform

os_map = {'Linux': 'linux', 'Darwin': 'macos', 'Windows': 'windows'}
arch_map = {
    'x86_64': 'x86_64', 'amd64': 'x86_64', 'AMD64': 'x86_64',
    'arm64': 'arm64', 'aarch64': 'aarch64',
}

system = platform.system()
machine = platform.machine()

try:
    os_name = os_map[system]
    arch_name = arch_map[machine]
except KeyError:
    print('unknown unknown', flush=True)
    raise SystemExit(1)

print(os_name, arch_name)
")"

if [ "$OS" = "unknown" ]; then
    echo "[pyspla] Cannot detect platform."
    echo "  system : $(python3 -c 'import platform; print(platform.system())')"
    echo "  machine: $(python3 -c 'import platform; print(platform.machine())')"
    echo ""
    echo "Supported: Linux (x86_64, aarch64), macOS (x86_64, arm64), Windows (x64)."
    exit 1
fi

# ------------------------------------------------------------------ choose wheel
case "$OS" in
    linux)
        WHEEL="pyspla-${VERSION}-py3-none-manylinux_2_38_${ARCH}.whl"
        ;;
    macos)
        if [ "$ARCH" = "arm64" ]; then
            WHEEL="pyspla-${VERSION}-py3-none-macosx_11_0_arm64.whl"
        else
            WHEEL="pyspla-${VERSION}-py3-none-macosx_10_15_x86_64.whl"
        fi
        ;;
    windows)
        # wheels on PyPI / GitHub keep 'win_amd64' tag
        WHEEL="pyspla-${VERSION}-py3-none-win_amd64.whl"
        ;;
    *)
        echo "[pyspla] Unsupported OS: $OS"
        exit 1
        ;;
esac

URL="https://github.com/${REPO}/releases/download/${TAG}/${WHEEL}"
TMP_DIR="$(mktemp -d)"
TMP_WHEEL="${TMP_DIR}/${WHEEL}"

echo "[pyspla] Platform detected : ${OS} ${ARCH}"
echo "[pyspla] Release tag       : ${TAG}"
echo "[pyspla] Installing wheel  : ${WHEEL}"

# ------------------------------------------------------------------ download
download() {
    if command -v curl &>/dev/null; then
        curl -fsSL -o "$TMP_WHEEL" "$URL"
    elif command -v wget &>/dev/null; then
        wget -q -O "$TMP_WHEEL" "$URL"
    else
        echo "[pyspla] Neither curl nor wget found. Install one and retry."
        exit 1
    fi
}

if ! download; then
    echo ""
    echo "[pyspla] Failed to download:"
    echo "  ${URL}"
    echo ""
    echo "Possible reasons:"
    echo "  - Release '${TAG}' does not exist yet."
    echo "  - Wheel '${WHEEL}' is not attached to that release."
    echo "  - Network / firewall issue."
    echo ""
    echo "You can inspect available assets at:"
    echo "  https://github.com/${REPO}/releases/tag/${TAG}"
    exit 1
fi

# ------------------------------------------------------------------ install
pip install "$TMP_WHEEL"

# ------------------------------------------------------------------ cleanup
rm -rf "$TMP_DIR"

# ------------------------------------------------------------------ verify
python3 -c "
import pyspla
print('[pyspla] Installed pyspla', pyspla.__version__, '— import OK')
"
