# Hebrew (RTL) support

This fork adds Hebrew support to crosspoint-reader.

Rendering works for Hebrew text in EPUBs — right-to-left layout, proper
word ordering, nikkud and cantillation marks attached to their base
letters, mixed Hebrew/Latin/number paragraphs with embedded LTR runs,
bidi bracket mirroring. Auto-detected from `dc:language`, `html dir=`,
or CSS `direction: rtl`. Hebrew UI strings (book titles, status bar)
go through a visual-order conversion.

Hebrew glyphs only ship in **Noto Sans** (reader font) and the UI fonts.
Bookerly and OpenDyslexic don't include them — pick Noto Sans in
settings if you're reading Hebrew. There is no Hebrew hyphenation
dictionary, no RTL page-turn swap, and the menu UI itself stays in
English.

Numbers in Hebrew text preserve LTR digit order ("8,000" reads
left-to-right inside an RTL paragraph). Trailing punctuation (commas,
periods, brackets) gets placed correctly per Unicode bidi.

## Architecture

All RTL logic is in [lib/Rtl/](./lib/Rtl/README.md). Upstream files
contain one-line hooks marked `RTL_FORK`:

```bash
grep -rn RTL_FORK src/ lib/
```

## Syncing with upstream

Upstream ships fonts without Hebrew glyphs; we layer NotoSansHebrew on
top during generation. The generated `.h` files are `merge=ours` in
`.gitattributes` so a sync never touches them — which means you have
to regenerate after each sync.

One-time:

```bash
git config merge.ours.driver true
pip install freetype-py==2.5.1
```

Each sync:

```bash
git fetch upstream
git merge upstream/master    # or rebase
bash scripts/regen-after-merge.sh    # or .ps1 on Windows
pio run
git add lib/EpdFont/builtinFonts/ src/fontIds.h
git commit -m "chore: regenerate fonts after upstream sync"
```

The regen script runs upstream's font conversion first (so any new
flags or output-format changes get picked up), then re-runs NotoSans /
Ubuntu / notosans_8 with the Hebrew fallback layered on via
`fontconvert.py --additional-intervals`, then rehashes `src/fontIds.h`.
