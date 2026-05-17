# Post-merge regeneration for this Hebrew fork (Windows PowerShell version).
#
# Run this after every `git merge upstream/master`. It rebuilds the
# generated files that `.gitattributes` kept as `merge=ours`, so the
# working tree ends up with upstream's latest source changes + our
# Hebrew fallback layered on top.
#
# Requires: python with freetype-py (pip install freetype-py==2.5.1),
# and bash on PATH (from Git for Windows) for the upstream font script.

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot

Write-Host "==> Regenerating fonts with Hebrew fallback"
Push-Location (Join-Path $repoRoot 'lib/EpdFont/scripts')
try {
  & (Join-Path $PWD 'convert-builtin-fonts-hebrew.ps1')
} finally {
  Pop-Location
}

Write-Host ""
Write-Host "==> Regenerating src/fontIds.h"
python (Join-Path $repoRoot 'scripts/gen_font_ids.py')

Write-Host ""
Write-Host "==> Done. Review changes with: git status"
