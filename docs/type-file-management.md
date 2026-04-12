# Type File Management

GreenPad allows users to create and delete keyword files (`.kwd`) and layout files (`.lay`)
directly from the Settings dialog, without leaving the application.

## Access

**Settings dialog** (`Ctrl+,` or menu: View > Settings) > **Document Types** section

Each row in the Document Types section has three buttons next to the keyword and layout fields:

| Button | Action |
|--------|--------|
| Add    | Create a new file in the `type\` directory |
| Del    | Delete the selected file from disk |
| edit   | Open the selected file for editing |

## Adding a File

Click **Add** next to the keyword or layout field.
A small dialog appears prompting for a base name (without extension):

```
Name: [________]   [OK] [Cancel]
```

- Enter only the base name, e.g. `C` to create `C.kwd` or `C.lay`.
- The extension is appended automatically.
- The file is created in the `type\` directory next to the executable.
- If the `type\` directory does not exist, it is created automatically.
- If a file with the same name already exists, the operation is silently ignored.

After creation the new filename is added to the combobox and selected.

## Generated Files

### Keyword File (`.kwd`)

Created with UTF-16LE encoding and BOM so that GreenPad recognizes the encoding
when the file is opened for editing.

Initial content (template):

```
0
/*
*/
//
```

Line meanings:

| Line | Content | Meaning |
|------|---------|---------|
| 1 | `0` or `1` | Case-sensitive matching (`1` = sensitive, `0` = insensitive) |
| 2 | e.g. `/*` | Block comment start symbol |
| 3 | e.g. `*/` | Block comment end symbol |
| 4 | e.g. `//` | Line comment symbol |
| 5+ | one keyword per line | Keywords to highlight |

Edit this file with GreenPad (click **edit**). Because the file starts with a UTF-16LE BOM,
GreenPad detects the encoding correctly and saves back in UTF-16LE.

### Layout File (`.lay`)

Created with UTF-16LE encoding and BOM, pre-filled with default display settings
(colors, font, tab width, wrap mode, etc.) derived from the current runtime defaults.

The initial content is equivalent to what the Layout dialog would produce for a brand-new
layout. Fields written:

| Key | Description |
|-----|-------------|
| `ct` | Text color (hex RGB) |
| `ck` | Keyword color |
| `cb` | Background color |
| `cr` | Read-only background color |
| `cc` | Comment color |
| `cn` | Special character color |
| `cl` | Line number color |
| `ft` | Font name |
| `sz` | Font size (pt) |
| `fw` | Font weight (omitted when `FW_DONTCARE`) |
| `ff` | Font flags: bit0=italic, bit1=underline, bit2=strikeout (omitted when 0) |
| `tb` | Tab width (characters) |
| `sc` | Special character visibility: 5 flags (EOF / newline / tab / space / full-width space) |
| `wp` | Wrap mode: `-1`=none, `0`=window width, positive=column width |
| `ww` | Wrap column width |
| `ws` | Smart wrap (`1`=word wrap, `0`=character wrap) |
| `ln` | Show line numbers (`1`=yes, `0`=no) |

Edit the layout visually with the **Edit Layout** dialog (click **edit**),
or edit the raw file directly with a text editor.

## Deleting a File

Click **Del** next to the keyword or layout field.
A confirmation dialog appears showing the filename.
On confirmation the file is deleted from disk and removed from the combobox.

> **Note:** Deleting a file that is currently referenced by a document type will leave
> that document type with a missing file. GreenPad will fall back to default settings
> for the missing file without error.

## File Location

All keyword and layout files are stored in:

```
<GreenPad executable directory>\type\
```

They are shared across all users of the same GreenPad installation.
