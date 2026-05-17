#!/bin/bash
# Post-merge regeneration for this Hebrew fork.
#
# Run this after every `git merge upstream/master`. It rebuilds the
# generated files that `.gitattributes` kept as `merge=ours`, so the
# working tree ends up with upstream's latest source changes + our
# Hebrew fallback layered on top.
#
# Requires: freetype-py (pip install freetype-py==2.5.1), python 3.

set -e

cd "$(dirname "$0")/.."

echo "==> Regenerating fonts with Hebrew fallback"
(cd lib/EpdFont/scripts && bash convert-builtin-fonts-hebrew.sh)

echo ""
echo "==> Regenerating src/fontIds.h"
python scripts/gen_font_ids.py

echo ""
echo "==> Done. Review changes with: git status"
