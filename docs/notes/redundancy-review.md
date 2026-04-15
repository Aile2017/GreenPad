# Redundancy and Cleanup Candidates

This note summarizes cleanup candidates found during a repository-wide review.
The items below are not urgent bug fixes, but they are good candidates for future maintenance work because they increase duplication, make intent harder to follow, or leave obsolete code paths in place.

## 1. Consolidate replacement expansion in `Search.cpp`

`SearchManager::ReplaceImpl()` and `SearchManager::ReplaceAllImpl()` both expand regular-expression replacement text with nearly identical logic.
They each call `DoExpandRepl()`, allocate a temporary buffer, write the expanded text, and add a null terminator.

**Why this matters**

- The replacement behavior is maintained in two places.
- Future fixes to backreference handling can easily drift.
- The duplicated flow makes the replace code longer than necessary.

**Suggested follow-up**

Extract the shared flow into a helper that returns the expanded replacement buffer and length.

## 2. Reduce duplicated dialog-to-`LayData` copying in `ConfigManager.cpp`

> **Resolved.** `DialogToLayData()` and `LayDataToDialog()` helpers were introduced.
> `ParseLayData()`, `SaveToFile()`, and the constructor all use these helpers.
> The constructor previously initialised some fields in the initialiser list and others
> manually from `LayData::SetDefaults()`; it was refactored to populate a `LayData`
> (applying `cfg.default*` overrides) and call `LayDataToDialog()` in one step.

## 3. Simplify duplicated combo-box text loading in `SearchManager::UpdateData()`

`UpdateData()` reads the find text and replace text with the same allocation and `GetItemText()` pattern twice.

**Why this matters**

- This is small duplication, but it adds noise to a central update routine.
- Any future change to history handling or text extraction must be repeated.

**Decision: left as-is.** The duplication spans only two call sites with five lines each.
A third combo box would tip the balance, but for now the overhead of adding a private
helper outweighs the benefit.

## 4. Revisit `SearchManager::NotFound()`

> **Resolved.** `NotFound()` was removed entirely.
> Its only caller was `ReplaceImpl()`, which always passed `GoingDown=false`, making
> the `GoingDown=true` branch dead code.  The single live line (`MsgBox IDS_NOTFOUND`)
> was inlined into `ReplaceImpl()` directly.

## 5. Clarify window size restoration logic in `ConfigManager.h`

> **Resolved.** `GetWndW()` and `GetWndH()` were refactored.
> The implicit fallback `wndPos_.right > wndPos_.left` was replaced with an explicit
> `rememberWindowSize_ || rememberWindowPlace_` condition, making it clear that the
> stored size is used as a hint whenever either restore flag is on.

## 6. Review legacy non-Unicode branches

> **Resolved in three steps.**
>
> All supported toolchains define `UNICODE` and `_UNICODE`; non-Unicode builds are not
> a current target.  Dead branches were removed across the codebase:
>
> **Step 1** — `ConfigManager.cpp`, `Search.cpp`
> - Removed the `#ifndef UNICODE` block containing the old `ToByte`/`GetColor`/`GetInt`
>   implementations.
> - Removed `#ifdef UNICODE` guards around `my_lstrcpysW` (font name copy) and
>   `GetPrivateProfileStringW`.
> - Removed `#ifdef _UNICODE` guard in `SearchManager::on_init()`.
>
> **Step 2** — `kilib/file.cpp`, `editwing/ip_draw.cpp`, `editwing/ip_cursor.cpp`
> - Removed `#ifdef UNICODE` guard wrapping `GetUNCPath()` and its call sites in
>   `CreateFileUNC()`, `GetFileAttributesUNC()`, and `FileW::Open()`.
> - Removed `#ifdef UNICODE` guard in the line-number rendering path.
> - Removed `#ifdef _UNICODE` guard in `Cursor::on_ime_reconvertstring()`.
>
> **Step 3** — `kilib/` headers and implementation files
> - `stdafx.h`: removed empty `#ifdef _UNICODE` block.
> - `kstring.h`: removed non-Unicode macro aliases, `XTCHAR` else-branch,
>   `operator+=` else-branch, and `#ifdef` guards on `FreeWCMem`/`FreeCMem`.
> - `string.cpp`: removed non-Unicode branches from `operator=`, `ConvToWChar`,
>   `ConvToChar`, `isCompatibleWithACP`, and `operator+=`.
> - `textfile.cpp`: inlined W-suffix API names; removed non-Unicode UTF-8/UTF-7 branch.
> - `registry.cpp`: removed non-Unicode branches from `GetPath` and `PutPath`.
> - `path.cpp`: removed `#ifndef _UNICODE` block (short-filename validation).
> - `winutil.cpp`: removed ANSI clipboard fallback and `DragQueryFileA` branch.
> - `window.cpp`: removed `SetCompositionFontA` branch.
> - `log.cpp`: removed `#ifdef _UNICODE` guard on BOM write.

## Priority Guidance

Item 1 (replacement expansion) remains open.  All other items have been resolved.
