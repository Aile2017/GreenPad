# GreenPad Keyboard Shortcuts

Complete reference of all keyboard shortcuts, organized by category.

---

## File Operations

| Key | Action |
|-----|--------|
| `Ctrl+N` | New file |
| `Ctrl+O` | Open file |
| `Ctrl+R` | Reopen current file |
| `Alt+Enter` | Reopen current file (alternate) |
| `Ctrl+L` | Open with elevated privileges (UAC) |
| `F5` | Reload file from disk |
| `Ctrl+S` | Save |
| `Ctrl+Shift+S` / `F12` | Save As |
| `Ctrl+E` | Save and exit |
| `Ctrl+Shift+E` | Discard changes and exit |
| `Ctrl+F4` | Exit |
| `Escape` | Quick exit (when enabled in settings) |

---

## Edit

| Key | Action |
|-----|--------|
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+X` / `Shift+Delete` | Cut |
| `Ctrl+C` / `Ctrl+Insert` | Copy |
| `Ctrl+V` / `Shift+Insert` | Paste |
| `Ctrl+A` | Select all |
| `Ctrl+I` | Insert Unicode code point |
| `F6` | Insert current date and time |

---

## Search and Navigation

| Key | Action |
|-----|--------|
| `Ctrl+F` / `Ctrl+H` | Open Find/Replace dialog |
| `F3` | Find next |
| `Shift+F3` / `F2` | Find previous |
| `Ctrl+J` | Jump to line number |
| `Ctrl+G` | External grep |
| `F7` | Open selection as file path or URL |
| `F8` | Show selection length |

---

## Text Transformation

| Key | Action |
|-----|--------|
| `Alt+U` | Convert selection to uppercase |
| `Alt+L` | Convert selection to lowercase |
| `Alt+I` | Invert case of selection |
| `Alt+R` | Reduce selection to ASCII (asciify) |
| `Alt+W` | Trim trailing whitespace |
| `Alt+Z` | Strip first characters |
| `Alt+A` | Strip last characters |
| `Alt+Q` | Quote selection |
| `Alt+N` | Unquote selection |

---

## Line and Range Deletion

| Key | Action |
|-----|--------|
| `Ctrl+K` | Delete from cursor to end of line |
| `Ctrl+U` | Delete from cursor to start of line |
| `Ctrl+Shift+K` | Delete from cursor to end of file |
| `Ctrl+Shift+U` | Delete from cursor to start of file |

---

## View and Zoom

| Key | Action |
|-----|--------|
| `Ctrl+1` | Disable word wrap |
| `Ctrl+2` | Wrap at fixed character width |
| `Ctrl+3` | Wrap at window edge |
| `Ctrl+0` | Reset zoom to 100% |
| `Ctrl+Up` | Zoom in (+10%) |
| `Ctrl+Down` | Zoom out (−10%) |

---

## Window Management

| Key | Action |
|-----|--------|
| `Ctrl+Tab` | Switch to next window |
| `Ctrl+Shift+Tab` | Switch to previous window |

---

## External Tools

| Key | Action |
|-----|--------|
| `Ctrl+\` | Run external filter on selection (see [External Filter Feature](external-filter-spec.md)) |
| `Ctrl+G` | External grep |
| `F1` | Help |

---

## Cursor Movement

All movement keys can be combined with `Shift` to extend the selection.

| Key | Action |
|-----|--------|
| `Left` / `Right` | Move one character |
| `Ctrl+Left` / `Ctrl+Right` | Move one word |
| `Up` / `Down` | Move one line |
| `Home` | Move to start of line |
| `End` | Move to end of line |
| `Ctrl+Home` | Move to start of document |
| `Ctrl+End` | Move to end of document |
| `Page Up` / `Page Down` | Move one page |

---

## Text Editing Keys

| Key | Action |
|-----|--------|
| `Delete` | Delete character to the right |
| `Ctrl+Delete` | Delete word to the right |
| `Backspace` | Delete character to the left |
| `Ctrl+Backspace` | Delete word to the left |
| `Insert` | Toggle insert / overwrite mode |
| `Tab` | Insert tab / increase indent |
| `Shift+Tab` | Decrease indent |
| `Ctrl+B` | Move to matching brace |
| `Shift+Ctrl+B` | Extend selection to matching brace |

---

## Read-only Mode

These keys are active when the file is opened in read-only mode (`-r` flag or `RO` status bar indicator).

| Key | Action |
|-----|--------|
| `Space` | Scroll down one page |
| `Shift+Space` | Scroll up one page |
| `Escape` / `Q` | Exit |
