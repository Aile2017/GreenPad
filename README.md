# GreenPad

A lightweight text editor for Windows, aiming to be a practical Notepad replacement with minimal footprint.

Original author: [k.inaba](http://www.kmonos.net/lib/gp.en.html)
Extended by: roytam1, [RamonUnch](https://github.com/RamonUnch/GreenPad)
This build: modernized 64-bit fork with PCRE2 and chardet support.

The original GreenPad supported a wide range of Windows versions down to Windows XP (x86/x64).
This fork targets 64-bit Windows only and drops all legacy compatibility code, requiring Windows Vista or later.

## Features

- Unicode 14.0 support
- Proportional font rendering
- Syntax highlighting (customizable via `.kwd` files)
- Regular expression search powered by PCRE2 (via `pcre2-16.dll`)
- Charset auto-detection via `chardet.dll` (optional, based on libchardet)
- Wide encoding support: UTF-8/16/32, EUC-JP, Shift-JIS, GB18030, and many more (see [docs/encodings.md](docs/encodings.md))
- Word wrap (character or word boundary)
- Smart indentation
- UAC elevation support
- UI languages: English, Japanese, Simplified Chinese, Traditional Chinese, Korean, Russian — extensible via `.lng` files (see [Command line](#command-line))
- External filter (`Ctrl+\`): pipe selected text (or the whole file) through any command-line program and replace it with the output — enables on-the-fly text transformation via `sort`, `sed`, `awk`, Perl, Python, Ruby, and more (see [docs/external-filter-spec.md](docs/external-filter-spec.md))

## System Requirements

- Windows Vista or later (x64)

## Optional DLLs

Place these DLLs in the same directory as `GreenPad.exe` to enable additional features:

| DLL | Purpose |
|-----|---------|
| `chardet.dll` | Charset auto-detection (libchardet-based, MPL/GPL/LGPL) |
| `pcre2-16.dll` | PCRE2 regex engine; falls back to built-in NFA if absent |

## Building

### Using Top-level Makefile

The top-level `Makefile` provides convenient targets for all toolchains:

```bash
make vcc      # Build with MSVC
make gcc64    # Build with MSYS2 mingw64
make clang64  # Build with MSYS2 CLANG64
```

### Visual C++ (x64)

**Preparation:**
- Use Developer Command Prompt for Native Tools (x64) from Visual Studio 2026
- Verify `nmake`, `cl`, and `link` are available in PATH

#### Build

```bat
nmake /f Makefiles/vcc.mak
```

#### Clean

```bat
nmake /f Makefiles/vcc.mak clean
```

### MSYS2 mingw64

**Preparation:**
- `C:\usr\msys64` must be installed
- Set `PATH` to `/mingw64/bin:/usr/bin` before building

#### Build

```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/mingw64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefiles/gcc64.mak"
```

#### Clean

```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/mingw64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefiles/gcc64.mak clean"
```

### MSYS2 UCRT64

**Preparation:**
- `C:\usr\msys64` must be installed
- Set `PATH` to `/ucrt64/bin:/usr/bin` before building

#### Build

```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/ucrt64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefiles/gcc64.mak"
```

#### Clean

```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/ucrt64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefiles/gcc64.mak clean"
```

### MSYS2 CLANG64

**Preparation:**
- `C:\usr\msys64` must be installed
- Set `PATH` to `/clang64/bin:/usr/bin` before building

#### Build

```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/clang64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefiles/clang64.mak"
```

#### Clean

```bash
C:/usr/msys64/usr/bin/bash.exe -c "export PATH=/clang64/bin:/usr/bin:$PATH; /usr/bin/make -f Makefiles/clang64.mak clean"
```

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl+R` | Reopen |
| `Ctrl+L` | Open elevated |
| `Shift+Ctrl+S` / `F12` | Save As |
| `Ctrl+Y` | Redo |
| `F5` | Reload file |
| `F6` | Insert date & time |
| `F7` | Open selection as file/URL |
| `F8` | Display selection length |
| `Ctrl+F` | Find |
| `F3` / `Shift+F3` | Find next / prev |
| `Ctrl+H` | Replace |
| `Ctrl+J` | Jump to line |
| `Ctrl+G` | External grep |
| `Ctrl+1/2/3` | Wrap: none / fixed width / window width |
| `Ctrl+I` | Insert Unicode code point |
| `Ctrl+B` | Go to matching brace |

For a complete list of all keyboard shortcuts, see [docs/keybindings.md](docs/keybindings.md).

## Configuration

### Layout files (`.lay`)

Located in `release/type/`. Each document type has its own `.lay` file.

**GUI editor:** open **View → Settings**, select a document type, then click the **"edit"** button next to the Layout dropdown. The dialog lets you pick colors with a color picker and choose the font interactively.

To edit manually, the file is UTF-16 LE with BOM; each line is `key=value`:

```
# Font
ft=Font name
sz=Font size (points)
fw=Font weight (0=don't care  400=normal  700=bold)
ff=Font style flags (1:Italic  2:Underline  4:Strikeout, combinable)
fx=Font width in points (0=default)
cs=Font charset (Windows LOGFONT lfCharSet; 0=DEFAULT_CHARSET)
fq=Font quality (Windows LOGFONT lfQuality; 0=DEFAULT_QUALITY)

# Colors (6-digit hex RGB, e.g. FF0000=red)
ct=Text color
ck=Keyword color
cb=Background color
cr=Read-only background color
cc=Comment color
cn=Special/control character color
cl=Line number color

# Tab / special characters
tb=Tab width (characters)
sc=BBBBB  (five 0/1 flags: EOF marker / line endings / tabs / spaces / fullwidth-spaces)

# Wrapping
wp=Wrap type (-1:none  0:window right edge  1:fixed width)
ww=Wrap width in characters (used when wp=1)
ws=Smart wrap (1=word boundaries  0=character)

# Line numbers
ln=Show line numbers (1=yes  0=no)
```

Color behavior: normal mode uses `cb`, read-only mode uses `cr`.

### Syntax highlighting (`.kwd`)

Place `.kwd` files in `release/type/`. Format:

```
1111       # flags: CaseSensitive EnSingleQuote EnDoubleQuote EnEscape
/*         # block comment start
*/         # block comment end
//         # line comment start
keyword1
keyword2
...
```

### Command line

```
greenpad [-r] [-l<line>] [-c<charset>] [-e<locale>] <file> ...
```

Options:

- `-r` Start in read-only mode
- `-l<line>` Open at the specified line number (e.g. `-l42`)
- `-c<charset>` Specify encoding (e.g. `-c65001` for UTF-8N). See [docs/encodings.md](docs/encodings.md) for the full list.
- `-e<locale>` Override UI language. The argument is a BCP 47 locale name corresponding to a `.lng` file placed in the same directory as `GreenPad.exe`.

Available locales (bundled):

| Locale | Language |
|---|---|
| `en-US` | English |
| `ja-JP` | Japanese |
| `zh-CN` | Simplified Chinese |
| `zh-TW` | Traditional Chinese |
| `ko-KR` | Korean |
| `ru-RU` | Russian |

Additional languages can be added by placing a `<locale>.lng` file alongside the executable. If `-e` is omitted, the UI language is detected automatically from the Windows display language setting.

Common charset codes: `-65001` (UTF-8), `-5`/`-6` (UTF-16 BE/LE), `-932` (EUC-JP), `-933` (ISO-2022-JP)

### Shared configuration

To share one `GreenPad.ini` across user accounts, add to the ini file:

```ini
[SharedConfig]
Enable=1
```

## Credits

### Application Icon

[Compose, create, edit, file, office, pencil, writing, creative Icon](https://icon-icons.com/icon/compose-create-edit-file-office-pencil-writing-creative/107746) by [icon-icons.com](https://icon-icons.com/), used under free for commercial use license.

## License

[CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/) — public domain dedication.
