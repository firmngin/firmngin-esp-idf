#!/usr/bin/env bash
# IDF purity gate: core sources must not call Arduino-only APIs directly.
# Arduino-only usage is allowed only inside src/fng_compat.h, the vendored IDF
# component, or inside `#if defined(ARDUINO)` fenced regions. Comments ignored.
set -euo pipefail
srcdir="$(cd "$(dirname "$0")/.." && pwd)/src"
python3 "$(cd "$(dirname "$0")" && pwd)/check_idf_purity.py" "$srcdir"
