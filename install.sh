#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

SIGN_ARGS=()
for arg in "$@"; do
    case "$arg" in
        --no-notarize) SIGN_ARGS+=(--no-notarize) ;;
    esac
done

detect_os() {
    case "$(uname -s)" in
        Darwin) echo "macos" ;;
        Linux)  echo "linux" ;;
        MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
        *) echo "unknown" ;;
    esac
}

OS="$(detect_os)"

echo "Cleaning..."
rm -rf Builds

echo "Building..."
case "$OS" in
    windows)
        cmd.exe //c build.bat clean Release
        ;;
    macos)
        cmake -S . -B Builds/Ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
        cmake --build Builds/Ninja -- -j"$(sysctl -n hw.logicalcpu)"
        ;;
    linux)
        cmake -S . -B Builds/Ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
        cmake --build Builds/Ninja -- -j"$(nproc)"
        ;;
    *)
        echo "Unsupported OS: $(uname -s)"
        exit 1
        ;;
esac

case "$OS" in
    macos)
        echo "Signing..."
        "$SCRIPT_DIR/../___sign___/sign.sh" "${SIGN_ARGS[@]}" "$SCRIPT_DIR/Builds/Ninja/cast_App_artefacts/Release/cast"
        ;;
esac

echo "Installing..."
case "$OS" in
    windows)
        ARTIFACT="Builds/Ninja/cast_App_artefacts/Release/cast.exe"
        INSTALL_DIR="$HOME/.local/bin"
        mkdir -p "$INSTALL_DIR"
        cp "$ARTIFACT" "$INSTALL_DIR/cast.exe"
        ;;
    macos)
        ARTIFACT="Builds/Ninja/cast_App_artefacts/Release/cast"
        INSTALL_DIR="$HOME/.local/bin"
        mkdir -p "$INSTALL_DIR"
        cp "$ARTIFACT" "$INSTALL_DIR/cast"
        ;;
    linux)
        ARTIFACT="Builds/Ninja/cast_App_artefacts/Release/cast"
        INSTALL_DIR="$HOME/.local/bin"
        mkdir -p "$INSTALL_DIR"
        cp "$ARTIFACT" "$INSTALL_DIR/cast"
        ;;
esac

echo "Done."
