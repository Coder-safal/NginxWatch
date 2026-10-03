#!/usr/bin/env bash
# Extract matching Debian/Ubuntu development headers without root access.
# An optional .deb argument lets offline users reuse an already downloaded package.
set -euo pipefail

if (( $# > 1 )); then
    echo "Usage: bash scripts/setup-local-curl.sh [path/to/libcurl4-openssl-dev.deb]" >&2
    exit 2
fi

package_path="${1:-}"
if [[ -n "$package_path" ]]; then
    package_path="$(realpath -- "$package_path")"
fi
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
project_dir="$PWD"

if ! runtime_version="$(dpkg-query -W -f='${Version}' libcurl4t64 2>/dev/null)"; then
    runtime_version="$(dpkg-query -W -f='${Version}' libcurl4 2>/dev/null)"
fi
if [[ -z "$runtime_version" ]]; then
    echo "Install the libcurl runtime or the system libcurl4-openssl-dev package first." >&2
    exit 1
fi

download_dir="$(mktemp -d -t nginxwatch-curl.XXXXXXXX)"
trap 'rm -rf -- "$download_dir"' EXIT
if [[ -z "$package_path" ]]; then
    (
        cd -- "$download_dir"
        apt download "libcurl4-openssl-dev=$runtime_version"
    )
    packages=("$download_dir"/*.deb)
    package_path="${packages[0]}"
fi

if [[ "$(dpkg-deb -f "$package_path" Package)" != "libcurl4-openssl-dev" ||
      "$(dpkg-deb -f "$package_path" Version)" != "$runtime_version" ||
      "$(dpkg-deb -f "$package_path" Architecture)" != "$(dpkg --print-architecture)" ]]; then
    echo "The development package must match this machine's libcurl runtime version and architecture." >&2
    exit 1
fi

mkdir -p -- "$project_dir/.deps/curl"
dpkg-deb -x "$package_path" "$project_dir/.deps/curl"
echo "Local libcurl headers are ready in .deps/curl. Run make -j2."
