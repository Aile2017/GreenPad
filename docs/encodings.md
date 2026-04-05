# Supported Encodings

All encodings supported by GreenPad, as defined in `kilib/textfile.h`.

The encoding can be specified via the `-c` command-line option (e.g. `-c65001` for UTF-8N).
Negative values denote variants with BOM or ISO-2022/EUC wrappers as noted below.

---

## Unicode

| Encoding | Code | Notes |
|---|---|---|
| UTF-8N | `65001` | No BOM |
| UTF-8 | `-65001` | With BOM |
| UTF-7 | `65000` | |
| UTF-16 LE | `-4` | With BOM |
| UTF-16 BE | `-3` | With BOM |
| UTF-16LE | `-6` | No BOM |
| UTF-16BE | `-5` | No BOM |
| UTF-32 LE | `-8` | With BOM |
| UTF-32 BE | `-7` | With BOM |
| UTF-32LE | `-10` | No BOM |
| UTF-32BE | `-9` | No BOM |
| UTF-1 | `-1` | No BOM |
| UTF-1 | `-64999` | With BOM |
| UTF-5 | `-2` | No BOM |
| UTF-5 | `-14` | With BOM |
| UTF-9 | `-11` | No BOM |
| UTF-9 | `-65002` | With BOM |
| Old FSS-UTF | `-12` | No BOM |
| Old FSS-UTF | `-13` | With BOM |
| UTF-EBCDIC | `-15` | No BOM |
| UTF-EBCDIC | `-16` | With BOM |
| SCSU | `-60002` | Standard Compression Scheme for Unicode, no BOM |
| SCSU | `-60000` | With BOM |
| BOCU-1 | `-60003` | Binary Ordered Compression for Unicode, no BOM |
| BOCU-1 | `-60001` | With BOM |

---

## Japanese

| Encoding | Code | Notes |
|---|---|---|
| Shift_JIS | `932` | |
| EUC-JP | `-932` | |
| ISO-2022-JP | `-933` | |
| x-mac-japanese | `10001` | |

---

## Chinese (Simplified)

| Encoding | Code | Notes |
|---|---|---|
| GB18030 | `54936` | No BOM |
| GB18030 | `-54936` | With BOM |
| GBK (EUC-CN) | `936` | |
| ISO-2022-CN | `-936` | |
| HZ-GB2312 | `-937` | |
| x-mac-prc | `10008` | |

## Chinese (Traditional)

| Encoding | Code | Notes |
|---|---|---|
| Big5 | `950` | |
| EUC-TW / CNS 11643 | `20000` | |
| TCA | `20001` | |
| ETen | `20002` | |
| IBM5550 | `20003` | |
| Teletext | `20004` | |
| Wang | `20005` | |
| x-mac-taiwan | `10002` | |

---

## Korean

| Encoding | Code | Notes |
|---|---|---|
| EUC-KR (UHC) | `949` | |
| ISO-2022-KR | `-950` | |
| Johab | `1361` | |
| x-mac-korean | `10003` | |

---

## Cyrillic

| Encoding | Code | Notes |
|---|---|---|
| Windows-1251 | `1251` | |
| ISO-8859-5 | `28595` | |
| KOI8-R | `20866` | |
| KOI8-U | `21866` | Ukrainian |
| CP855 (IBM) | `855` | |
| CP866 (MS-DOS) | `866` | |
| x-mac-cyrillic | `10007` | |
| x-mac-ukraine | `10017` | Ukrainian |

---

## Western European

| Encoding | Code | Notes |
|---|---|---|
| Windows-1252 | `1252` | Superset of ISO-8859-1 |
| ISO-8859-15 | `28605` | |
| CP850 (MS-DOS) | `850` | |
| x-mac-roman | `10000` | |
| DOSLatinUS (CP437) | `437` | |

---

## Central European

| Encoding | Code | Notes |
|---|---|---|
| Windows-1250 | `1250` | |
| ISO-8859-2 | `28592` | |
| CP852 (MS-DOS) | `852` | |
| x-mac-ce | `10029` | |
| x-mac-romania | `10010` | Romanian |
| x-mac-croatia | `10082` | Croatian |

---

## Greek

| Encoding | Code | Notes |
|---|---|---|
| Windows-1253 | `1253` | |
| ISO-8859-7 | `28597` | |
| CP737 (MS-DOS) | `737` | |
| CP869 (MS-DOS) | `869` | |
| x-mac-greek | `10006` | |

---

## Turkish

| Encoding | Code | Notes |
|---|---|---|
| Windows-1254 | `1254` | |
| ISO-8859-9 | `28599` | |
| CP857 (MS-DOS) | `857` | |
| x-mac-turkish | `10081` | |

---

## Hebrew

| Encoding | Code | Notes |
|---|---|---|
| Windows-1255 | `1255` | |
| CP862 (MS-DOS) | `862` | |
| x-mac-hebrew | `10005` | |

---

## Arabic

| Encoding | Code | Notes |
|---|---|---|
| Windows-1256 | `1256` | |
| ISO-8859-6 | `28596` | |
| CP720 | `720` | |
| CP864 (MS-DOS) | `864` | |
| x-mac-arabic | `10004` | |

---

## Other

| Encoding | Code | Notes |
|---|---|---|
| Auto-detect | `0` | SJIS / EUC-JP / ISO-2022-JP / EUC-KR / ISO-2022-CN / UTF-5/8/16/32 |
| ASCII | `20127` | Plain ASCII (CP20127) |
| Baltic (Windows-1257) | `1257` | |
| Baltic (CP775, MS-DOS) | `775` | |
| Vietnamese (Windows-1258) | `1258` | |
| Thai (Windows-874) | `874` | |
| Thai (ISO-8859-11) | `28601` | |
| Esperanto (ISO-8859-3) | `28593` | |
| Portuguese (CP860) | `860` | |
| Icelandic (CP861) | `861` | |
| Icelandic (x-mac-icelandic) | `10079` | |
| Canadian French (CP863) | `863` | |
| Nordic (CP865) | `865` | |
| Nordic MS-DOS (CP865) | `865` | |
