# EOL Symbol Display Issue

## Current Implementation

File: `editwing/ip_draw.cpp` (around line 887)

```cpp
static const unicode eolSymTbl[3][2] = {
    { L'\xFFE9', L'\0' }, // CR
    { L'\xFFEC', L'\0' }, // LF
    { L'\x21B2', L'\0' }, // CRLF
};
```

| Line ending | Code point | Character | Unicode block |
|-------------|------------|-----------|---------------|
| CR   | U+FFE9 | ← (fullwidth left arrow)           | Halfwidth and Fullwidth Forms |
| LF   | U+FFEC | ↓ (fullwidth downwards arrow)      | Halfwidth and Fullwidth Forms |
| CRLF | U+21B2 | ↲ (downwards arrow with tip left)  | Arrows |

## Problem

U+FFE9 and U+FFEC are in the "Halfwidth and Fullwidth Forms" block and are not present
in many non-CJK fonts. U+21B2 is also absent from many common fonts. As a result, the
EOL symbols may not render correctly depending on the user's selected font.

## Options Considered

### Option 1: Draw symbols via GDI (font-independent)
Draw arrows/lines using GDI primitives instead of characters.
- Pro: Guaranteed to render regardless of font.
- Con: Higher implementation cost.

### Option 2: Switch to more widely supported Unicode characters

| Line ending | Candidate | Code point | Notes |
|-------------|-----------|------------|-------|
| CR   | `←` | U+2190 | Basic Arrows block, broad support |
| LF   | `↓` | U+2193 | Basic Arrows block, broad support |
| CRLF | `↵` | U+21B5 | Arrows block, moderate support |
| CRLF | `↙` | U+2199 | Basic Arrows block, broader support |

### Option 3: Render symbols using a fallback font
Create a separate font handle (e.g., Segoe UI Symbol or Lucida Sans Unicode) and use it
only for EOL symbol characters, keeping the user's chosen font for normal text.

## Implementation (completed)

Adopted Option 2 with INI-based customization.

- Default symbol: U+2193 (↓) for all three line-ending types.
- Users can override via hidden INI parameters using `U+XXXX` notation:
  ```
  EolSymCR=U+FFE9
  EolSymLF=U+FFEC
  EolSymCRLF=U+21B2
  ```
- Keys are read from the active INI section (same section as other settings).
- Values outside the `U+XXXX` format silently fall back to the default.

### Key files changed
- `editwing/ewCommon.h` — added `unicode eolSym[3]` to `VConfig`
- `editwing/ip_view.h` — added `unicode eolSym_[3]` and `eolSym(int)` accessor to `Painter`
- `editwing/ip_draw.cpp` — `Painter::Init()` copies from `VConfig::eolSym`; `DrawTXT` uses `p.eolSym(lbType_)`
- `ConfigManager.h/.cpp` — reads INI, syncs to all doctypes including the default

### Pitfalls encountered
1. **Stale .o files**: Modifying `ip_view.h` or `ewCommon.h` requires a full `make clean`
   rebuild — the Makefile has no header dependency tracking.
2. **Default doctype not refreshed**: The default doctype is loaded before `LoadIni()` reads
   the EOL symbols. `SetDocTypeByName()` skips `LoadLayout()` for already-loaded doctypes,
   so an explicit sync of `eolSym_` into the default doctype is needed inside `LoadIni()`.
