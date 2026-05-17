# lib/Rtl

RTL/Hebrew support for the reader. All logic lives here so upstream files
only carry one-line hooks tagged `RTL_FORK`. To find every integration
point: `grep -rn RTL_FORK src/ lib/`.

## Files

- `Rtl.h` — public API
- `RtlBidi.cpp` — `toVisualOrder`, `wordIsRtl`, `wordHasLtrContent`, `reverseGraphemeClusters`, `mirrorBrackets`
- `RtlDirection.cpp` — language code → isRtl
- `RtlParserHook.cpp` — `ScopedAttach`, `onStartElement`, `resolveBlockStyle`, `setInheritedDirection`
- `RtlWordEmitter.cpp` — bracket splitting and grapheme reversal for word emission
- `RtlLineLayout.cpp` — line x-position computation for RTL lines

## Where the hooks are called

`Rtl::Direction::fromLanguageCode` + `Rtl::ParserHook::ScopedAttach` — once in `Section.cpp` before the parser runs.

`Rtl::ParserHook::onStartElement` — once in `ChapterHtmlSlimParser::startElement`, after class/style/id are extracted.

`Rtl::ParserHook::resolveBlockStyle` — after each `BlockStyle::fromCssStyle` call in `ChapterHtmlSlimParser` (3 call sites).

`Rtl::ParserHook::setInheritedDirection` — for manually-built BlockStyles (centered, table cell, paragraph default).

`Rtl::WordEmitter::emit` — once in `flushPartWordBuffer`, replacing the inline bracket/reversal logic.

`Rtl::LineLayout::positionLineRtl` — once in `ParsedText::extractLine` (RTL branch).

`Rtl::LineLayout::isNaturalAlignment` — three places in `ParsedText.cpp` (first-line-indent gating).

`Rtl::toVisualOrder` — `GfxRenderer::truncatedText`, `GfxRenderer::wrappedText`, `BaseTheme::drawStatusBar`.

## Structural diffs we don't hide behind hooks

A few things have to live on upstream types directly. All additive, marked `RTL_FORK`:

- `bool isRtl` on `BlockStyle` and its serialize/deserialize in `TextBlock.cpp`
- `BlockStyle::getCombinedBlockStyle` inheritance: `combinedBlockStyle.isRtl = child.isRtl || isRtl`
- `CssDirection` enum, `direction` field + `directionDefined` sibling bool, `hasDirection()`, `Start`/`End` enum values, parse + serialize in `CssParser.cpp`, cache version bump in `CssParser.h`
- Hebrew combining-mark ranges in `Utf8.h::utf8IsCombiningMark`
- `SECTION_FILE_VERSION` bump in `Section.cpp`

## Upstream merge protocol

```bash
git fetch upstream
git merge upstream/master   # or rebase if you prefer linear history
# Conflicts should appear only at RTL_FORK marker lines. Real logic
# conflicts mean upstream restructured something we depend on — investigate.

bash scripts/regen-after-merge.sh         # or .ps1 on Windows
pio run
```

Font binaries (`lib/EpdFont/builtinFonts/*.h`) and `src/fontIds.h` are
`merge=ours` so upstream font changes never touch them — the regen
script rebuilds them with the Hebrew fallback layered in. Hebrew
codepoint ranges go through upstream's `fontconvert.py` via
`--additional-intervals`, so the script itself stays vanilla.

## Performance

- `WordEmitter::emit` short-circuits for LTR blocks (one bool check)
- `LineLayout::positionLineRtl` is only called when `blockStyle.isRtl`
- Parser state is thread-local; no pointer in `ChapterHtmlSlimParser`
- `toVisualOrder` short-circuits when no RTL codepoint is present
