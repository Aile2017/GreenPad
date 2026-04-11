# Layout Dialog

The layout dialog provides a GUI editor for the display settings stored in a `.lay` file.  
Open it from **Edit Layout** in the settings dialog for the layout file associated with the selected document type.

## Target Files

- Load source: `type\*.lay` next to the executable
- Save destination: `type\*.lay` next to the executable
- Save format: UTF-16LE with BOM

When loading, the dialog uses `ConfigManager::GetLayData()`, so if the file does not exist under `type\`, it can also read a section with the same name from `GreenPad.ini`.  
When you press **OK**, the dialog always writes out a real file under `type\`.

## Editable Items

### 1. Colors

The dialog edits these seven color entries:

| UI label | `.lay` key | Meaning |
|---|---|---|
| Text Color | `ct` | Normal text color |
| Keyword Color | `ck` | Keyword color |
| Background | `cb` | Normal background color |
| BG (Read-Only) | `cr` | Background color in read-only mode |
| Comment | `cc` | Comment color |
| Special Char | `cn` | Special character color |
| Line Number | `cl` | Line number color |

Each color control is owner-drawn and shows its current value as a `RRGGBB` hex string. Clicking it opens the standard Windows color picker.

If `cl` is missing in the layout file, the loader falls back to the text color (`ct`).

### 2. Font / Tab

- **Font...**  
  Opens the standard Windows font dialog and updates font name, size, bold, italic, underline, and strikeout.
- **Tab**  
  Sets the tab width. On save, the value is clamped to the range `1` to `64`.

The font display field shows `font name + style + size`.  
Example: `Consolas Bold 10`

### 3. Special Chars

These checkboxes control the `sc` value:

| Checkbox | `sc` bit position |
|---|---|
| EOF | 1st digit |
| NewLine | 2nd digit |
| Tab | 3rd digit |
| Half-SP | 4th digit |
| Full-SP | 5th digit |

`sc` is saved as a five-character `0` / `1` string.  
Example: `sc=11100`

### 4. Wrap

The dialog supports three wrap modes:

| Option | `.lay` key | Meaning |
|---|---|---|
| None | `wp=-1` | No wrapping |
| Right Edge | `wp=0` | Wrap at the window edge |
| Width | `wp=1`, `ww=<value>` | Wrap at a fixed column |

The `ww` input and the `Word` / `Char` choices are enabled only when **Width** is selected.

- **Word** -> `ws=1`
- **Char** -> `ws=0`

`ww` is clamped to at least `1` on save.

### 5. Show Line Numbers

- **Show Line Numbers** -> `ln=1` or `ln=0`

## Default Values

If the layout file is missing, or if some values are not specified, the dialog starts from the same built-in defaults used by `ConfigManager::LoadLayout()`.

- Background color: Windows `COLOR_WINDOW`
- Text color and line number color: Windows `COLOR_WINDOWTEXT`
- Read-only background color: `DefaultReadOnlyBgColor()`
- Tab width: `4`
- Special character visibility: `sc=11100`
- Wrapping: `wp=-1`, `ww=80`, `ws=1`
- Line numbers: enabled
- Font: application default font

## Save Behavior

When you press **OK**, the dialog rewrites the layout file using only the fields it manages.  
That means manually maintained entries like these are not preserved:

- `fx` : font width
- `cs` : character set
- `fq` : font quality
- comment lines (`;...`) and any unsupported keys

If you need those values, edit the `.lay` file manually after saving from the dialog.

## Error Handling

If saving fails, the dialog shows **Failed to save layout file.** and stays open.
