#!/bin/bash
set -e

VST3_PATH="${1:-build/MeltVerb_artefacts/Debug/VST3/MeltVerb.vst3}"
STRICTNESS="${2:-5}"

if [ ! -d "$VST3_PATH" ]; then
    echo "FAILED: VST3 not found at $VST3_PATH"
    exit 1
fi

echo "Validating: $VST3_PATH (strictness=$STRICTNESS)"

if command -v pluginval &> /dev/null; then
    pluginval --validate "$VST3_PATH" --strictness-level "$STRICTNESS"
    echo "PASSED"
else
    echo "pluginval not found. Skipping."
fi
