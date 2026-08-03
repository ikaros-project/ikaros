#!/bin/sh
set -eu

repository_root=$(CDPATH= cd -- "$(dirname -- "$0")/../../../.." && pwd)
environment="$repository_root/.venv-aliked-lightglue"

python3 -m venv "$environment"
"$environment/bin/pip" install -r "$(dirname -- "$0")/requirements.txt"
TORCH_HOME="$repository_root/.model-cache" \
    "$environment/bin/python" "$(dirname -- "$0")/download_models.py"

echo "Run Ikaros with: -p $environment/bin/python"
