#!/bin/bash
set -e

VST3_PATH="build/MeltVerb_artefacts/Debug/VST3/MeltVerb.vst3"
STRICTNESS=${1:-5}

if [ ! -d "$VST3_PATH" ]; then
    echo "ERROR: VST3 not found at $VST3_PATH"
    echo "Run 'cmake --build build/' first."
    exit 1
fi

if command -v pluginval &> /dev/null; then
    pluginval --validate "$VST3_PATH" --strictness-level "$STRICTNESS"
else
    echo "pluginval not found in PATH. Skipping validation."
    echo "Install from: https://github.com/Tracktion/pluginval"
fi
