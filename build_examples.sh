#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
examples_dir="$script_dir/examples"

if [[ ! -d "$examples_dir" ]]; then
    echo "[build-examples] examples/ directory not found; nothing to build." >&2
    exit 0
fi

build_all() {
    local status=0
    for example in "$examples_dir"/*; do
        [[ -d "$example" ]] || continue
        if [[ -f "$example/platformio.ini" ]]; then
            echo "[build-examples] Building ${example##*/}" >&2
            if ! pio run -d "$example"; then
                status=$?
                break
            fi
        fi
    done
    return "$status"
}

if build_all; then
    echo "[build-examples] All example projects built successfully." >&2
    exit 0
else
    echo "[build-examples] Example build failed." >&2
    exit 1
fi
